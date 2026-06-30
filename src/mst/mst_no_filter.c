#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mst.h"

#define BUFFER_SIZE_MB 32
#define BUFFER_EDGES ((BUFFER_SIZE_MB * 1024 * 1024) / sizeof(Edge))

static void MST_ComputeComponentMinEdgesNoFilter(FILE *readFile, Edge *readBuffer, unsigned int *edgeCount,
                                                 Edge *componentMinEdges, DisjointSetPool *disjointSetPool) {
  unsigned int updatedEdgeCount = 0;

  unsigned int readCount;
  while ((readCount = fread(readBuffer, sizeof(Edge), BUFFER_EDGES, readFile)) > 0) {
    for (unsigned int edgeId = 0; edgeId < readCount; edgeId++) {
      const Edge edge = readBuffer[edgeId];

      const NodeId sourceRootId = DisjointSet_Find(disjointSetPool, edge.source);
      const NodeId destRootId = DisjointSet_Find(disjointSetPool, edge.destination);

      if (sourceRootId == destRootId)
        continue;

      updatedEdgeCount++;

      Edge *componentMinEdgeOnSource = &componentMinEdges[sourceRootId];
      Edge *componentMinEdgeOnDest = &componentMinEdges[destRootId];

      if (edge.weight < componentMinEdgeOnSource->weight) {
        *componentMinEdgeOnSource = edge;
      }
      if (edge.weight < componentMinEdgeOnDest->weight) {
        *componentMinEdgeOnDest = edge;
      }
    }
  }

  *edgeCount = updatedEdgeCount;
}

void MST_ComputeNoFilter(const char *graphFilePath, unsigned int edgeCount, unsigned int nodeCount, Edge *mstEdgeList,
                         unsigned int *mstCount, Weight *mstWeightSum) {
  if (mstWeightSum) {
    *mstWeightSum = 0;
  }

  DisjointSetPool disjointSetPool;
  if (!DisjointSet_Initialize(&disjointSetPool, nodeCount)) {
    fprintf(stderr, "Erro: MST: falha ao inicializar conjunto disjunto\n");
    return;
  }

  Edge *componentMinEdges = malloc(sizeof(Edge) * nodeCount);
  Edge *readBuffer = malloc(sizeof(Edge) * BUFFER_EDGES);

  if (!componentMinEdges || !readBuffer) {
    fprintf(stderr, "Erro: MST: falha ao alocar buffers principais: %s\n", strerror(errno));
    if (componentMinEdges) {
      free(componentMinEdges);
    }
    if (readBuffer) {
      free(readBuffer);
    }
    DisjointSet_Finalize(&disjointSetPool);
    return;
  }

  unsigned int currentEdgeCount = edgeCount;
  while (disjointSetPool.componentCount > 1 && currentEdgeCount > 0) {
    FILE *readFile = fopen(graphFilePath, "rb");
    if (!readFile) {
      fprintf(stderr, "Erro: MST: falha ao abrir arquivo de entrada: %s\n", strerror(errno));
      break;
    }

    MST_ResetComponentMinEdges(componentMinEdges, nodeCount);
    MST_ComputeComponentMinEdgesNoFilter(readFile, readBuffer, &currentEdgeCount, componentMinEdges, &disjointSetPool);
    fclose(readFile);

    MST_ApplyComponentMinEdges(componentMinEdges, &disjointSetPool, nodeCount, mstEdgeList, mstCount);
  }
  *mstWeightSum = disjointSetPool.componentSums[disjointSetPool.lastUnifiedRootId];

  DisjointSet_Finalize(&disjointSetPool);
  free(readBuffer);
  free(componentMinEdges);
}
