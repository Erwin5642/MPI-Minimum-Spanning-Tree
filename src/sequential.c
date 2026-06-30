#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "core/graph.h"
#include "core/mst_output.h"
#include "mst/mst.h"

int main(int argc, char **argv) {
  if (argc < 4) {
    fprintf(stderr, "Uso: %s <arquivo_grafo> <numero_de_nos> <numero_de_arrestas>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  char *graphFilePath = argv[1];
  unsigned int nodeCount = atoi(argv[2]);
  unsigned int edgeCount = atoi(argv[3]);

  clock_t startTime = clock();

  Edge *mstEdgeList = malloc(sizeof(Edge) * (nodeCount - 1));
  if (!mstEdgeList) {
    fprintf(stderr, "Erro: Sequential: falha ao alocar árvore geradora mínima: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  unsigned int mstCount = 0;

  Weight mstWeightSum = 0;
  MST_Compute(graphFilePath, edgeCount, nodeCount, mstEdgeList, &mstCount, &mstWeightSum);

  clock_t endTime = clock();
  double elapsedSeconds = ((double)(endTime - startTime)) / CLOCKS_PER_SEC;

  printf("===== MST Computada (Sequencial) =====\n");
  printf("Total de arestas na MST: %u\n", mstCount);
  printf("Contagem esperada: %u\n", nodeCount - 1);
  printf("Soma: %.17g\n", mstWeightSum);
  printf("Tempo de execução (CPU): %f segundos\n", elapsedSeconds);
  if (!MST_Output_WriteToTxt("mst_sequential.txt", mstEdgeList, mstCount)) {
    free(mstEdgeList);
    exit(EXIT_FAILURE);
  }

  free(mstEdgeList);

  return 0;
}
