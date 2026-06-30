#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/mst/mst.h"
#include "mst_debug.h"

#define BUFFER_SIZE_MB 32
#define BUFFER_EDGES ((BUFFER_SIZE_MB * 1024 * 1024) / sizeof(Edge))

static void MST_Debug_ResetComponentMinEdges(Edge *componentMinEdges, unsigned int nodeCount) {
  for (NodeId rootNodeId = 0; rootNodeId < nodeCount; rootNodeId++) {
    componentMinEdges[rootNodeId] = GRAPH_NOTHING;
  }
}

static void MST_Debug_ComputeComponentMinEdgesNoFilter(FILE *readFile, Edge *readBuffer, unsigned int *edgeCount,
                                                       Edge *componentMinEdges, DisjointSetPool *disjointSetPool) {
  unsigned int updatedEdgeCount = 0;

  unsigned int readCount;
  while ((readCount = fread(readBuffer, sizeof(Edge), BUFFER_EDGES, readFile)) > 0) {
    for (unsigned int edgeId = 0; edgeId < readCount; edgeId++) {
      const Edge edge = readBuffer[edgeId];

      const NodeId sourceRootId = DisjointSet_Find(disjointSetPool, edge.source);
      const NodeId destRootId = DisjointSet_Find(disjointSetPool, edge.destination);

      if (sourceRootId == destRootId) {
        continue;
      }

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

static unsigned int MST_Debug_ApplyComponentMinEdges(Edge *componentMinEdges, DisjointSetPool *disjointSetPool,
                                                     unsigned int nodeCount, Edge *mstEdgeList, unsigned int *mstCount) {
  unsigned int unionCount = 0;

  for (NodeId rootNodeId = 0; rootNodeId < nodeCount; rootNodeId++) {
    const Edge edge = componentMinEdges[rootNodeId];

    if (edge.weight == GRAPH_NOTHING.weight) {
      continue;
    }

    if (DisjointSet_Union(disjointSetPool, edge.source, edge.destination, edge.weight)) {
      mstEdgeList[(*mstCount)++] = edge;
      unionCount++;
    }
  }

  return unionCount;
}

void MST_ComputeDebug(const char *graphFilePath, unsigned int edgeCount, unsigned int nodeCount, Edge *mstEdgeList,
                      unsigned int *mstCount, Weight *mstWeightSum) {
  if (mstWeightSum) {
    *mstWeightSum = 0;
  }

  DisjointSetPool disjointSetPool;
  if (!DisjointSet_Initialize(&disjointSetPool, nodeCount)) {
    fprintf(stderr, "Erro: MST Debug: falha ao inicializar conjunto disjunto\n");
    return;
  }

  Edge *componentMinEdges = malloc(sizeof(Edge) * nodeCount);
  Edge *readBuffer = malloc(sizeof(Edge) * BUFFER_EDGES);

  if (!componentMinEdges || !readBuffer) {
    fprintf(stderr, "Erro: MST Debug: falha ao alocar buffers principais: %s\n", strerror(errno));
    free(componentMinEdges);
    free(readBuffer);
    DisjointSet_Finalize(&disjointSetPool);
    return;
  }

  unsigned int currentEdgeCount = edgeCount;
  unsigned int phase = 0;

  while (disjointSetPool.componentCount > 1 && currentEdgeCount > 0) {
    // TODO(debug-remove): remover log de início de fase.
    printf("[progresso][sequencial-debug] Iniciando fase %u: componentes=%u arestas=%u\n", phase,
           disjointSetPool.componentCount, currentEdgeCount);

    FILE *readFile = fopen(graphFilePath, "rb");
    if (!readFile) {
      fprintf(stderr, "Erro: MST Debug: falha ao abrir arquivo de entrada na fase %u: %s\n", phase, strerror(errno));
      break;
    }

    MST_Debug_ResetComponentMinEdges(componentMinEdges, nodeCount);
    MST_Debug_ComputeComponentMinEdgesNoFilter(readFile, readBuffer, &currentEdgeCount, componentMinEdges,
                                               &disjointSetPool);

    fclose(readFile);

    // TODO(debug-remove): remover log após filtragem.
    printf("[progresso][sequencial-debug] Fase %u: arestas após filtragem=%u\n", phase, currentEdgeCount);

    unsigned int unionsApplied = MST_Debug_ApplyComponentMinEdges(componentMinEdges, &disjointSetPool, nodeCount,
                                                                  mstEdgeList, mstCount);

    // TODO(debug-remove): remover log após uniões.
    printf("[progresso][sequencial-debug] Fase %u: unioes aplicadas=%u componentes=%u mstCount=%u\n", phase,
           unionsApplied, disjointSetPool.componentCount, *mstCount);

    phase++;
  }

  if (disjointSetPool.componentCount > 1 && currentEdgeCount == 0) {
    // TODO(debug-remove): remover log de estagnação.
    printf("[progresso][sequencial-debug] Encerrado sem progresso: componentes=%u e arestas=0\n",
           disjointSetPool.componentCount);
  }

  // TODO(debug-remove): remover log de finalização.
  printf("[progresso][sequencial-debug] Finalizado: fases=%u componentes=%u mstCount=%u\n", phase,
         disjointSetPool.componentCount, *mstCount);

  if (mstWeightSum && disjointSetPool.lastUnifiedRootId != GRAPH_NODE_ID_MAX) {
    *mstWeightSum = disjointSetPool.componentSums[disjointSetPool.lastUnifiedRootId];
  }

  DisjointSet_Finalize(&disjointSetPool);
  free(readBuffer);
  free(componentMinEdges);
}
