#ifndef DISJOINTSET_H

#define DISJOINTSET_H

#include <stdbool.h>

#include "graph.h"

typedef struct {
  NodeId *parents;
  unsigned int *ranks;
  unsigned int nodesCount;
  unsigned int componentsCount;  
} DisjointSetPool;

bool DisjointSet_Initialize(DisjointSetPool *pool, const unsigned int nodeCount);
void DisjointSet_Finalize(DisjointSetPool *pool);
NodeId DisjointSet_Find(DisjointSetPool *pool, const NodeId target);
bool DisjointSet_Union(DisjointSetPool *pool, const NodeId first, const NodeId second);

#endif
