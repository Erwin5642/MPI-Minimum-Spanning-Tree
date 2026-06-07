#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mst.h"

void MST_ResetComponentMinEdges(Edge *componentMinEdges, unsigned int nodeCount) {
  for (NodeId rootNodeId = 0; rootNodeId < nodeCount; rootNodeId++) {
    componentMinEdges[rootNodeId] = GRAPH_NOTHING;
  }
}

void MST_ComputeComponentMinEdges(Edge *edgeList, unsigned int *edgeCount, Edge *componentMinEdges,
                                  DisjointSetPool *disjointSetPool) {
  unsigned int updatedEdgeCount = 0;

  for (EdgeId edgeId = 0; edgeId < *edgeCount; edgeId++) {
    const Edge edge = edgeList[edgeId];

    const NodeId sourceRootId = DisjointSet_Find(disjointSetPool, edge.source);
    const NodeId destRootId = DisjointSet_Find(disjointSetPool, edge.destination);

    if (sourceRootId == destRootId)
      continue;

    edgeList[updatedEdgeCount++] = edge;

    Edge *componentMinEdgeOnSource = &componentMinEdges[sourceRootId];
    Edge *componentMinEdgeOnDest = &componentMinEdges[destRootId];

    if (edge.weight < componentMinEdgeOnSource->weight) {
      *componentMinEdgeOnSource = edge;
    }
    if (edge.weight < componentMinEdgeOnDest->weight) {
      *componentMinEdgeOnDest = edge;
    }
  }
  *edgeCount = updatedEdgeCount;
}

void MST_ApplyComponentMinEdges(Edge *componentMinEdges, DisjointSetPool *disjointSetPool, unsigned int nodeCount,
                                Edge *mstEdgeList, unsigned int *mstCount) {
  for (NodeId rootNodeId = 0; rootNodeId < nodeCount; rootNodeId++) {
    const Edge edge = componentMinEdges[rootNodeId];

    if (edge.weight == GRAPH_NOTHING.weight)
      continue;

    if (DisjointSet_Union(disjointSetPool, edge.source, edge.destination)) {
      mstEdgeList[(*mstCount)++] = edge;
    }
  }
}

bool MST_Compute(Edge *edgeList, unsigned int *edgeCount, unsigned int nodeCount, Edge *mstEdgeList,
                 unsigned int *mstCount) {
  DisjointSetPool disjointSetPool;
  if (!DisjointSet_Initialize(&disjointSetPool, nodeCount)) {
    fprintf(stderr, "Erro: MST: falha ao inicializar conjunto disjunto\n");
    return false;
  }

  Edge *componentMinEdges = malloc(sizeof(Edge) * nodeCount);
  if (!componentMinEdges) {
    fprintf(stderr, "Erro: MST: falha ao alocar arestas mínimas por componente: %s\n", strerror(errno));
    DisjointSet_Finalize(&disjointSetPool);
    return false;
  }

  while (disjointSetPool.componentCount > 1) {
    MST_ResetComponentMinEdges(componentMinEdges, nodeCount);
    MST_ComputeComponentMinEdges(edgeList, edgeCount, componentMinEdges, &disjointSetPool);
    MST_ApplyComponentMinEdges(componentMinEdges, &disjointSetPool, nodeCount, mstEdgeList, mstCount);
  }

  DisjointSet_Finalize(&disjointSetPool);
  free(componentMinEdges);
  return true;
}
