#include <errno.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/graph.h"
#include "mpi/mst_mpi.h"
#include "mpi/types.h"

#define CHUNK_SIZE_MB 64
#define CHUNK_EDGES ((CHUNK_SIZE_MB * 1024 * 1024) / sizeof(Edge))

void Parallel_ComputeCounts(unsigned int edgeCount, int mpiWorldSize, unsigned int *counts) {
  unsigned int baseEdgeCount = edgeCount / mpiWorldSize;
  unsigned int remainderEdgeCount = edgeCount % mpiWorldSize;

  for (int rankId = 0; rankId < mpiWorldSize; rankId++) {
    counts[rankId] = baseEdgeCount + ((unsigned int)rankId < remainderEdgeCount ? 1 : 0);
  }
}

void Parallel_SendEdgesToRankAsync(FILE *graphFile, int rankId, unsigned int edgeCount) {
  Edge *buffers[2] = {malloc(sizeof(Edge) * CHUNK_EDGES), malloc(sizeof(Edge) * CHUNK_EDGES)};
  if (!buffers[0] || !buffers[1]) {
    fprintf(stderr, "Erro: Parallel: falha ao alocar buffers para envio assíncrono em lote: %s\n", strerror(errno));
    free(buffers[0]);
    free(buffers[1]);
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }

  MPI_Request requests[2] = {MPI_REQUEST_NULL, MPI_REQUEST_NULL};
  unsigned int sentCount = 0;
  unsigned int chunkId = 0;
  int slotId = 0;

  while (sentCount < edgeCount) {
    if (requests[slotId] != MPI_REQUEST_NULL) {
      MPI_Wait(&requests[slotId], MPI_STATUS_IGNORE);
      requests[slotId] = MPI_REQUEST_NULL;
    }

    unsigned int batchCount = edgeCount - sentCount;
    if (batchCount > CHUNK_EDGES) {
      batchCount = CHUNK_EDGES;
    }

    fread(buffers[slotId], sizeof(Edge), batchCount, graphFile);
    MPI_Isend(buffers[slotId], batchCount, MPI_EDGE_TYPE, rankId, 0, MPI_COMM_WORLD, &requests[slotId]);

    sentCount += batchCount;
    chunkId++;
    // TODO(debug-remove): remove log below (parallel.c, next line); removable vars for this log: chunkId.
    printf("[progress][distribution][root->rank=%d] chunks=%u sent=%u/%u edges\n", rankId, chunkId, sentCount,
           edgeCount);
    slotId = 1 - slotId;
  }

  if (requests[0] != MPI_REQUEST_NULL) {
    MPI_Wait(&requests[0], MPI_STATUS_IGNORE);
  }
  if (requests[1] != MPI_REQUEST_NULL) {
    MPI_Wait(&requests[1], MPI_STATUS_IGNORE);
  }

  free(buffers[0]);
  free(buffers[1]);
}

Edge *Parallel_DistributeEdges(const char *graphFilePath, unsigned int edgeCount, int mpiRank, int mpiWorldSize,
                               unsigned int *localEdgeCount) {
  unsigned int *rankEdgeCounts = malloc(sizeof(unsigned int) * mpiWorldSize);
  if (!rankEdgeCounts) {
    fprintf(stderr, "Erro: Parallel: falha ao alocar buffers de distribuição: %s\n", strerror(errno));
    free(rankEdgeCounts);
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }

  Parallel_ComputeCounts(edgeCount, mpiWorldSize, rankEdgeCounts);
  *localEdgeCount = rankEdgeCounts[mpiRank];
  // TODO(debug-remove): remove log below (parallel.c, next line); no removable vars (all reused in core logic).
  printf("[progress][distribution][rank=%d] assignedLocalEdges=%u totalEdges=%u worldSize=%d\n", mpiRank,
         *localEdgeCount, edgeCount, mpiWorldSize);

  Edge *localEdgeList = NULL;
  localEdgeList = malloc(sizeof(Edge) * *localEdgeCount);
  if (!localEdgeList) {
    fprintf(stderr, "Erro: Parallel: falha ao alocar buffer local de arestas: %s\n", strerror(errno));
    free(rankEdgeCounts);
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }

  if (mpiRank == 0) {
    FILE *graphFile = fopen(graphFilePath, "rb");
    if (!graphFile) {
      fprintf(stderr, "Erro: Parallel: falha ao abrir arquivo de grafo: %s\n", strerror(errno));
      free(localEdgeList);
      free(rankEdgeCounts);
      MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    fread(localEdgeList, sizeof(Edge), rankEdgeCounts[0], graphFile);
    // TODO(debug-remove): remove log below (parallel.c, next line); no removable vars.
    printf("[progress][distribution][rank=0] loadedOwnEdges=%u/%u\n", rankEdgeCounts[0], edgeCount);

    for (int rankId = 1; rankId < mpiWorldSize; rankId++) {
      Parallel_SendEdgesToRankAsync(graphFile, rankId, rankEdgeCounts[rankId]);
    }

    fclose(graphFile);
    // TODO(debug-remove): remove log below (parallel.c, next line); no removable vars.
    printf("[progress][distribution][rank=0] finishedSendingToAllRanks\n");
  } else {
    unsigned int receivedCount = 0;
    unsigned int recvChunkId = 0;
    while (receivedCount < *localEdgeCount) {
      unsigned int batchCount = *localEdgeCount - receivedCount;
      if (batchCount > CHUNK_EDGES) {
        batchCount = CHUNK_EDGES;
      }

      MPI_Recv(&localEdgeList[receivedCount], (int)batchCount, MPI_EDGE_TYPE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      receivedCount += batchCount;
      recvChunkId++;
      // TODO(debug-remove): remove log below (parallel.c, next line); removable vars for this log: recvChunkId.
      printf("[progress][distribution][rank=%d] recvChunks=%u received=%u/%u edges\n", mpiRank, recvChunkId,
             receivedCount, *localEdgeCount);
    }
  }

  // TODO(debug-remove): remove log below (parallel.c, next line); no removable vars.
  printf("[progress][distribution][rank=%d] distributionComplete localEdges=%u\n", mpiRank, *localEdgeCount);
  free(rankEdgeCounts);
  return localEdgeList;
}

int main(int argc, char **argv) {
  if (argc < 4) {
    fprintf(stderr, "Uso: %s <arquivo_grafo> <numero_de_nos> <numero_de_arrestas>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  char *graphFilePath = argv[1];
  unsigned int nodeCount = atoi(argv[2]);
  unsigned int edgeCount = atoi(argv[3]);

  MPI_Init(&argc, &argv);

  MPI_Register_Datatypes();

  int mpiRank, mpiWorldSize;
  MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpiWorldSize);
  // TODO(debug-remove): remove log below (parallel.c, next line); no removable vars.
  printf("[progress][main][rank=%d] mpiReady worldSize=%d\n", mpiRank, mpiWorldSize);

  unsigned int localEdgeCount;
  Edge *localEdgeList = Parallel_DistributeEdges(graphFilePath, edgeCount, mpiRank, mpiWorldSize, &localEdgeCount);
  // TODO(debug-remove): remove log below (parallel.c, next line); no removable vars.
  printf("[progress][main][rank=%d] distributionDone localEdgeCount=%u\n", mpiRank, localEdgeCount);

  Edge *mstEdgeList = malloc(sizeof(Edge) * (nodeCount - 1));
  if (!mstEdgeList) {
    fprintf(stderr, "Erro: Parallel: falha ao alocar buffer da árvore geradora mínima: %s\n", strerror(errno));
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }
  unsigned int mstCount = 0;

  double startTime = MPI_Wtime();

  MST_ComputeParallel(localEdgeList, &localEdgeCount, nodeCount, mstEdgeList, &mstCount);
  // TODO(debug-remove): remove log below (parallel.c, next line); no removable vars.
  printf("[progress][main][rank=%d] mstComputeDone localEdgeCount=%u mstCount=%u\n", mpiRank, localEdgeCount,
         mstCount);

  double endTime = MPI_Wtime();
  double elapsedSeconds = endTime - startTime;
  if (mpiRank == 0) {
    long double mstWeightSum = 0;
    printf("===== MST Computada (Paralela) =====\n");
    for (unsigned int mstEdgeId = 0; mstEdgeId < mstCount; mstEdgeId++) {
      mstWeightSum += mstEdgeList[mstEdgeId].weight;
    }

    printf("Total de arestas na MST: %u\n", mstCount);
    printf("Contagem esperada: %u\n", nodeCount - 1);
    printf("Soma: %Lf\n", mstWeightSum);
    printf("Tempo de execução: %f segundos\n", elapsedSeconds);
    printf("Resolução temporal: %e\n", MPI_Wtick());
  }

  free(localEdgeList);
  free(mstEdgeList);
  MPI_Free_Datatypes();
  MPI_Finalize();
  return 0;
}
