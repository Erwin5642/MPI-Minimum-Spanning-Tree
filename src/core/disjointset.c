#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "disjointset.h"

bool DisjointSet_Initialize(DisjointSetPool *pool, const unsigned int nodeCount) {
  pool->nodeCount = nodeCount;
  pool->componentCount = nodeCount;
  pool->rankValues = calloc(nodeCount, sizeof(unsigned int));
  pool->parentIds = malloc(sizeof(NodeId) * nodeCount);

  if (!pool->rankValues || !pool->parentIds) {
    fprintf(stderr, "Erro: DisjointSet: falha ao alocar memória: %s\n", strerror(errno));
    free(pool->rankValues);
    free(pool->parentIds);
    pool->parentIds = pool->rankValues = NULL;
    return false;
  }

  NodeId *parentIds = pool->parentIds;

  for (unsigned int nodeId = 0; nodeId < nodeCount; nodeId++) {
    parentIds[nodeId] = nodeId;
  }

  return true;
}

void DisjointSet_Finalize(DisjointSetPool *pool) {
  pool->nodeCount = 0;
  pool->componentCount = 0;
  free(pool->rankValues);
  free(pool->parentIds);
  pool->parentIds = pool->rankValues = NULL;
}

NodeId DisjointSet_Find(DisjointSetPool *pool, const NodeId nodeId) {
  NodeId currentNodeId = nodeId;
  NodeId *parentIds = pool->parentIds;

  while (currentNodeId != parentIds[currentNodeId]) {
    parentIds[currentNodeId] = parentIds[parentIds[currentNodeId]];
    currentNodeId = parentIds[currentNodeId];
  }

  return currentNodeId;
}

bool DisjointSet_Union(DisjointSetPool *pool, const NodeId firstNodeId, const NodeId secondNodeId) {
  if (firstNodeId >= pool->nodeCount || secondNodeId >= pool->nodeCount) {
    fprintf(stderr, "Erro: DisjointSet: NodeId inválido passado para Union: %u, %u\n", firstNodeId, secondNodeId);
    return false;
  }

  NodeId firstRootId = DisjointSet_Find(pool, firstNodeId);
  NodeId secondRootId = DisjointSet_Find(pool, secondNodeId);

  if (firstRootId == secondRootId)
    return false;

  NodeId *parentIds = pool->parentIds;
  unsigned int *rankValues = pool->rankValues;

  if (rankValues[firstRootId] < rankValues[secondRootId]) {
    NodeId tempRootId = firstRootId;
    firstRootId = secondRootId;
    secondRootId = tempRootId;
  }

  parentIds[secondRootId] = firstRootId;

  if (rankValues[firstRootId] == rankValues[secondRootId]) {
    rankValues[firstRootId]++;
  }

  pool->componentCount--;

  return true;
}
