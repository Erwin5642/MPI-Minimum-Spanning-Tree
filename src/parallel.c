#include <errno.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/graph.h"
#include "core/mst.h"
#include "io/stream.h"
#include "mpi/types.h"

Edge *Parallel_DistributeEdges(Edge *globalEdgeList, unsigned int edgeCount, int mpiRank, int mpiWorldSize,
                               unsigned int *localEdgeCount) {
  int *scatterCounts = malloc(sizeof(int) * mpiWorldSize);
  int *scatterDisplacements = malloc(sizeof(int) * mpiWorldSize);
  if (!scatterCounts || !scatterDisplacements) {
    fprintf(stderr, "Erro: Parallel: falha ao alocar buffers de distribuição: %s\n", strerror(errno));
    free(scatterCounts);
    free(scatterDisplacements);
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }

  int baseEdgeCount = edgeCount / mpiWorldSize;
  int remainderEdgeCount = edgeCount % mpiWorldSize;
  int edgeOffset = 0;

  for (int rankId = 0; rankId < mpiWorldSize; rankId++) {
    scatterCounts[rankId] = baseEdgeCount + (rankId < remainderEdgeCount ? 1 : 0);
    scatterDisplacements[rankId] = edgeOffset;
    edgeOffset += scatterCounts[rankId];
  }

  *localEdgeCount = scatterCounts[mpiRank];
  Edge *localEdgeList = malloc(sizeof(Edge) * *localEdgeCount);
  if (!localEdgeList) {
    fprintf(stderr, "Erro: Parallel: falha ao alocar buffer local de arestas: %s\n", strerror(errno));
    free(scatterCounts);
    free(scatterDisplacements);
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }

  MPI_Scatterv(globalEdgeList, scatterCounts, scatterDisplacements, MPI_EDGE_TYPE, localEdgeList,
               scatterCounts[mpiRank], MPI_EDGE_TYPE, 0, MPI_COMM_WORLD);

  free(scatterCounts);
  free(scatterDisplacements);

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

  Edge *globalEdgeList = NULL;
  size_t fileSizeBytes;
  if (mpiRank == 0) {
    globalEdgeList = Stream_Map(graphFilePath, &fileSizeBytes);
    if (!globalEdgeList)
      MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }

  unsigned int localEdgeCount;
  Edge *localEdgeList = Parallel_DistributeEdges(globalEdgeList, edgeCount, mpiRank, mpiWorldSize, &localEdgeCount);

  if (mpiRank == 0) {
    Stream_Unmap(globalEdgeList, fileSizeBytes);
  }

  Edge *mstEdgeList = malloc(sizeof(Edge) * (nodeCount - 1));
  if (!mstEdgeList) {
    fprintf(stderr, "Erro: Parallel: falha ao alocar buffer da árvore geradora mínima: %s\n", strerror(errno));
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }
  unsigned int mstCount = 0;

  MPI_Barrier(MPI_COMM_WORLD);

  double startTime = MPI_Wtime();

  if (!MST_ComputeParallel(localEdgeList, &localEdgeCount, nodeCount, mstEdgeList, &mstCount, MPI_COMM_WORLD))
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);

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
