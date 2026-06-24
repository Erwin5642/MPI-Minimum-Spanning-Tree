#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "core/graph.h"
#include "mst/mst.h"

int main(int argc, char **argv) {
  if (argc < 4) {
    fprintf(stderr, "Uso: %s <arquivo_grafo> <numero_de_nos> <numero_de_arrestas>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  char *graphFilePath = argv[1];
  unsigned int nodeCount = atoi(argv[2]);
  unsigned int edgeCount = atoi(argv[3]);

  Edge *mstEdgeList = malloc(sizeof(Edge) * (nodeCount - 1));
  if (!mstEdgeList) {
    fprintf(stderr, "Erro: Sequential: falha ao alocar árvore geradora mínima: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  unsigned int mstCount = 0;

  clock_t startTime = clock();

  MST_Compute(graphFilePath, edgeCount, nodeCount, mstEdgeList, &mstCount);

  clock_t endTime = clock();
  double elapsedSeconds = ((double)(endTime - startTime)) / CLOCKS_PER_SEC;

  long double mstWeightSum = 0;
  printf("===== MST Computada (Sequencial) =====\n");
  for (unsigned int mstEdgeId = 0; mstEdgeId < mstCount; mstEdgeId++) {
    mstWeightSum += mstEdgeList[mstEdgeId].weight;
  }

  printf("Total de arestas na MST: %u\n", mstCount);
  printf("Contagem esperada: %u\n", nodeCount - 1);
  printf("Soma: %Lf\n", mstWeightSum);
  printf("Tempo de execução (CPU): %f segundos\n", elapsedSeconds);

  free(mstEdgeList);

  return 0;
}
