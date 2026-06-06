#include <stdlib.h>
#include <stdio.h>

#include "disjointset.h"

bool DisjointSet_Initialize(DisjointSetPool *pool, const unsigned int nodeCount) {
  pool->nodesCount = nodeCount;
  pool->componentsCount = nodeCount;
  pool->ranks = calloc(nodeCount, sizeof(unsigned int));
  pool->parents = malloc(sizeof(NodeId) * nodeCount);

  if (!pool->ranks || !pool->parents) return false; 

  NodeId *parents = pool->parents;

  for (unsigned int i = 0; i < nodeCount; i++) {
    parents[i] = i;
  }

  return true;
}

void DisjointSet_Finalize(DisjointSetPool *pool) {
  pool->nodesCount = 0;
  pool->componentsCount = 0;
  free(pool->ranks);
  free(pool->parents);
  pool->parents = pool->ranks = NULL;
}

NodeId DisjointSet_Find(DisjointSetPool *pool, const NodeId target) {
  NodeId currentNode = target;
  NodeId *parent = pool->parents;

  while (currentNode != parent[currentNode]) {
    parent[currentNode] = parent[parent[currentNode]];
    currentNode = parent[currentNode];
  }

  return currentNode;
}

bool DisjointSet_Union(DisjointSetPool *pool, const NodeId first, const NodeId second) {
  if (first >= pool->nodesCount || second >= pool->nodesCount) {
        fprintf(stderr, "CRITICAL: Invalid NodeId passed to Union: %u, %u\n", first, second);
        exit(EXIT_FAILURE);
    }
  NodeId rootFirst = DisjointSet_Find(pool, first);
  NodeId rootSecond = DisjointSet_Find(pool, second);

  if (rootFirst == rootSecond)
    return false;

  NodeId *parents = pool->parents;
  unsigned int *ranks = pool->ranks;

  if (ranks[rootFirst] < ranks[rootSecond]) {
    NodeId temp = rootFirst;
    rootFirst = rootSecond;
    rootSecond = temp;
  }

  parents[rootSecond] = rootFirst;

  if (ranks[rootFirst] == ranks[rootSecond]) {
    ranks[rootFirst]++;
  }

  pool->componentsCount--;

  return true;
}
