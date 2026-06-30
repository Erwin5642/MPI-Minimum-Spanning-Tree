#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "mst_output.h"

bool MST_Output_WriteToTxt(const char *outputPath, const Edge *mstEdgeList, unsigned int mstCount) {
  FILE *outputFile = fopen(outputPath, "w");
  if (!outputFile) {
    fprintf(stderr, "Erro: MST_Output: falha ao abrir arquivo '%s': %s\n", outputPath, strerror(errno));
    return false;
  }

  for (unsigned int edgeId = 0; edgeId < mstCount; edgeId++) {
    const Edge edge = mstEdgeList[edgeId];
    if (fprintf(outputFile, "%u %.12f %u\n", edge.source, edge.weight, edge.destination) < 0) {
      fprintf(stderr, "Erro: MST_Output: falha ao escrever arquivo '%s': %s\n", outputPath, strerror(errno));
      fclose(outputFile);
      return false;
    }
  }

  if (fclose(outputFile) != 0) {
    fprintf(stderr, "Erro: MST_Output: falha ao fechar arquivo '%s': %s\n", outputPath, strerror(errno));
    return false;
  }

  return true;
}
