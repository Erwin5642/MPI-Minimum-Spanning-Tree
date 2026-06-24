#include <errno.h>
#include <mpi.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/graph.h"
#include "mpi/mst_mpi.h"
#include "mpi/types.h"

static void Parallel_ComputeCountsAndDisplacements(unsigned int edgeCount, int mpiWorldSize, unsigned int *counts,
                                                   unsigned int *displacements) {
  unsigned int baseEdgeCount = edgeCount / mpiWorldSize;
  unsigned int remainderEdgeCount = edgeCount % mpiWorldSize;
  unsigned int edgeOffset = 0;

  for (int rankId = 0; rankId < mpiWorldSize; rankId++) {
    counts[rankId] = baseEdgeCount + ((unsigned int)rankId < remainderEdgeCount ? 1 : 0);
    displacements[rankId] = edgeOffset;
    edgeOffset += counts[rankId];
  }
}

Edge *Parallel_DistributeEdges(const char *graphFilePath, unsigned int edgeCount, int mpiRank, int mpiWorldSize,
                               unsigned int *localEdgeCount) {
  unsigned int *rankEdgeCounts = malloc(sizeof(unsigned int) * mpiWorldSize);
  unsigned int *rankDisplacements = malloc(sizeof(unsigned int) * mpiWorldSize);
  if (!rankEdgeCounts || !rankDisplacements) {
    fprintf(stderr, "Erro: Parallel: falha ao alocar buffers de distribuição: %s\n", strerror(errno));
    free(rankEdgeCounts);
    free(rankDisplacements);
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }

  Parallel_ComputeCountsAndDisplacements(edgeCount, mpiWorldSize, rankEdgeCounts, rankDisplacements);

  *localEdgeCount = rankEdgeCounts[mpiRank];
  Edge *localEdgeList = NULL;
  if (*localEdgeCount > 0) {
    localEdgeList = malloc(sizeof(Edge) * *localEdgeCount);
    if (!localEdgeList) {
      fprintf(stderr, "Erro: Parallel: falha ao alocar buffer local de arestas: %s\n", strerror(errno));
      free(rankEdgeCounts);
      free(rankDisplacements);
      MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
  }

  if (mpiRank == 0) {
    FILE *graphFile = fopen(graphFilePath, "rb");
    if (!graphFile) {
      fprintf(stderr, "Erro: Parallel: falha ao abrir arquivo de grafo: %s\n", strerror(errno));
      free(localEdgeList);
      free(rankEdgeCounts);
      free(rankDisplacements);
      MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    Edge *readBuffer = malloc(sizeof(Edge) * *localEdgeCount);
    if (!readBuffer) {
      fprintf(stderr, "Erro: Parallel: falha ao alocar buffer de leitura em lote: %s\n", strerror(errno));
      fclose(graphFile);
      free(localEdgeList);
      free(rankEdgeCounts);
      free(rankDisplacements);
      MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    fread(localEdgeList, sizeof(Edge), rankEdgeCounts[0], graphFile);

    for (int rankId = 1; rankId < mpiWorldSize; rankId++) {
      fread(readBuffer, sizeof(Edge), rankEdgeCounts[rankId], graphFile);
      MPI_Send(readBuffer, rankEdgeCounts[rankId], MPI_EDGE_TYPE, rankId, 0, MPI_COMM_WORLD);
    }

    free(readBuffer);
    fclose(graphFile);
  } else {
    MPI_Recv(localEdgeList, *localEdgeCount, MPI_EDGE_TYPE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  }

  free(rankEdgeCounts);
  free(rankDisplacements);
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

  unsigned int localEdgeCount;
  Edge *localEdgeList = Parallel_DistributeEdges(graphFilePath, edgeCount, mpiRank, mpiWorldSize, &localEdgeCount);

  Edge *mstEdgeList = malloc(sizeof(Edge) * (nodeCount - 1));
  if (!mstEdgeList) {
    fprintf(stderr, "Erro: Parallel: falha ao alocar buffer da árvore geradora mínima: %s\n", strerror(errno));
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }
  unsigned int mstCount = 0;

  MPI_Barrier(MPI_COMM_WORLD);

  double startTime = MPI_Wtime();

  MST_ComputeParallel(localEdgeList, &localEdgeCount, nodeCount, mstEdgeList, &mstCount);

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
