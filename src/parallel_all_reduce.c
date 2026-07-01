#include <errno.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/graph.h"
#include "core/mst_output.h"
#include "mpi/mst_mpi_all_reduce.h"
#include "mpi/types.h"

#define CHUNK_SIZE_MB 16
#define CHUNK_EDGES ((CHUNK_SIZE_MB * 1024 * 1024) / sizeof(Edge))

static void Parallel_ComputeCounts(unsigned int edgeCount, int mpiWorldSize, unsigned int *counts) {
  unsigned int baseEdgeCount = edgeCount / mpiWorldSize;
  unsigned int remainderEdgeCount = edgeCount % mpiWorldSize;

  for (int rankId = 0; rankId < mpiWorldSize; rankId++) {
    counts[rankId] = baseEdgeCount + ((unsigned int)rankId < remainderEdgeCount ? 1 : 0);
  }
}

static void Parallel_SendEdgesToRankAsync(FILE *graphFile, int rankId, unsigned int edgeCount) {
  Edge *buffers[2] = {malloc(sizeof(Edge) * CHUNK_EDGES), malloc(sizeof(Edge) * CHUNK_EDGES)};
  if (!buffers[0] || !buffers[1]) {
    fprintf(stderr, "Erro: Parallel AllReduce: falha ao alocar buffers para envio: %s\n", strerror(errno));
    free(buffers[0]);
    free(buffers[1]);
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }

  MPI_Request requests[2] = {MPI_REQUEST_NULL, MPI_REQUEST_NULL};
  unsigned int sentCount = 0;
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
    MPI_Isend(buffers[slotId], (int)batchCount, MPI_EDGE_TYPE, rankId, 0, MPI_COMM_WORLD, &requests[slotId]);

    sentCount += batchCount;
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

static Edge *Parallel_DistributeEdges(const char *graphFilePath, unsigned int edgeCount, int mpiRank, int mpiWorldSize,
                                      unsigned int *localEdgeCount) {
  unsigned int *rankEdgeCounts = malloc(sizeof(unsigned int) * mpiWorldSize);
  if (!rankEdgeCounts) {
    fprintf(stderr, "Erro: Parallel AllReduce: falha ao alocar buffers de distribuição: %s\n", strerror(errno));
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }

  Parallel_ComputeCounts(edgeCount, mpiWorldSize, rankEdgeCounts);
  *localEdgeCount = rankEdgeCounts[mpiRank];

  Edge *localEdgeList = malloc(sizeof(Edge) * *localEdgeCount);
  if (!localEdgeList) {
    fprintf(stderr, "Erro: Parallel AllReduce: falha ao alocar buffer local de arestas: %s\n", strerror(errno));
    free(rankEdgeCounts);
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }

  if (mpiRank == 0) {
    FILE *graphFile = fopen(graphFilePath, "rb");
    if (!graphFile) {
      fprintf(stderr, "Erro: Parallel AllReduce: falha ao abrir arquivo de grafo: %s\n", strerror(errno));
      free(localEdgeList);
      free(rankEdgeCounts);
      MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    fread(localEdgeList, sizeof(Edge), rankEdgeCounts[0], graphFile);

    for (int rankId = 1; rankId < mpiWorldSize; rankId++) {
      Parallel_SendEdgesToRankAsync(graphFile, rankId, rankEdgeCounts[rankId]);
    }

    fclose(graphFile);
  } else {
    unsigned int receivedCount = 0;
    while (receivedCount < *localEdgeCount) {
      unsigned int batchCount = *localEdgeCount - receivedCount;
      if (batchCount > CHUNK_EDGES) {
        batchCount = CHUNK_EDGES;
      }

      MPI_Recv(&localEdgeList[receivedCount], (int)batchCount, MPI_EDGE_TYPE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      receivedCount += batchCount;
    }
  }

  free(rankEdgeCounts);
  return localEdgeList;
}

int main(int argc, char **argv) {
  if (argc < 4) {
    fprintf(stderr, "Uso: %s <arquivo_grafo> <numero_de_nos> <numero_de_arrestas>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  char *graphFilePath = argv[1];
  unsigned int nodeCount = (unsigned int)atoi(argv[2]);
  unsigned int edgeCount = (unsigned int)atoi(argv[3]);

  MPI_Init(&argc, &argv);
  MPI_Register_Datatypes();

  int mpiRank, mpiWorldSize;
  MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpiWorldSize);

  double startTime = MPI_Wtime();

  Edge *mstEdgeList = malloc(sizeof(Edge) * (nodeCount - 1));
  if (!mstEdgeList) {
    fprintf(stderr, "Erro: Parallel AllReduce: falha ao alocar buffer da MST: %s\n", strerror(errno));
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }
  unsigned int mstCount = 0;

  unsigned int localEdgeCount = 0;
  double communicationStartTime = MPI_Wtime();
  Edge *localEdgeList = Parallel_DistributeEdges(graphFilePath, edgeCount, mpiRank, mpiWorldSize, &localEdgeCount);
  double communicationElapsedSeconds = MPI_Wtime() - communicationStartTime;

  Weight mstWeightSum = 0;
  double mstComputeStartTime = MPI_Wtime();
  MST_ComputeParallelAllReduce(localEdgeList, &localEdgeCount, nodeCount, mstEdgeList, &mstCount, &mstWeightSum);
  double mstComputeElapsedSeconds = MPI_Wtime() - mstComputeStartTime;

  double elapsedSeconds = MPI_Wtime() - startTime;
  MPI_Barrier(MPI_COMM_WORLD);
  printf("[all-reduce] Rank: %d Tempo de comunicação: %lf segundos\n", mpiRank, communicationElapsedSeconds);
  MPI_Barrier(MPI_COMM_WORLD);
  printf("[all-reduce] Rank: %d Tempo de computação MST: %lf segundos\n", mpiRank, mstComputeElapsedSeconds);
  MPI_Barrier(MPI_COMM_WORLD);

  if (mpiRank == 0) {
    printf("===== MST Computada (Paralela AllReduce) =====\n");
    printf("Total de arestas na MST: %u\n", mstCount);
    printf("Contagem esperada: %u\n", nodeCount - 1);
    printf("Soma: %.17g\n", mstWeightSum);
    printf("Tempo de execução: %f segundos\n", elapsedSeconds);
    if (!MST_Output_WriteToTxt("mst_parallel_all_reduce.txt", mstEdgeList, mstCount)) {
      free(localEdgeList);
      free(mstEdgeList);
      MPI_Free_Datatypes();
      MPI_Finalize();
      exit(EXIT_FAILURE);
    }
  }

  free(localEdgeList);
  free(mstEdgeList);
  MPI_Free_Datatypes();
  MPI_Finalize();
  return 0;
}
