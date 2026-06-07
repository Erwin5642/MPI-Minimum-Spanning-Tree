#ifndef DISJOINTSET_H

#define DISJOINTSET_H

#include <stdbool.h>

#include "graph.h"

typedef struct {
  NodeId *parentIds;
  unsigned int *rankValues;
  unsigned int nodeCount;
  unsigned int componentCount;
} DisjointSetPool;

bool DisjointSet_Initialize(DisjointSetPool *pool, const unsigned int nodeCount);
void DisjointSet_Finalize(DisjointSetPool *pool);
NodeId DisjointSet_Find(DisjointSetPool *pool, const NodeId nodeId);
bool DisjointSet_Union(DisjointSetPool *pool, const NodeId firstNodeId, const NodeId secondNodeId);

#endif
