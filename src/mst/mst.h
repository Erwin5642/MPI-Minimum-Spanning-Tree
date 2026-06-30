#ifndef MST_H
#define MST_H

#include <stdbool.h>
#include <stdio.h>

#include "../core/disjointset.h"
#include "../core/graph.h"

void MST_ResetComponentMinEdges(Edge *componentMinEdges, unsigned int nodeCount);
void MST_ComputeComponentMinEdges(FILE *readFile, Edge *readBuffer, unsigned int *edgeCount, Edge *componentMinEdges,
                                  DisjointSetPool *disjointSetPool);
void MST_ApplyComponentMinEdges(Edge *componentMinEdges, DisjointSetPool *disjointSetPool, unsigned int nodeCount,
                                Edge *mstEdgeList, unsigned int *mstCount);

void MST_Compute(const char *graphFilePath, unsigned int edgeCount, unsigned int nodeCount, Edge *mstEdgeList,
                 unsigned int *mstCount, Weight *mstWeightSum);

#endif
