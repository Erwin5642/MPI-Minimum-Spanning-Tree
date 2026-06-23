#ifndef MST_H
#define MST_H

#include <stdbool.h>
#include <mpi.h>

#include "disjointset.h"
#include "graph.h"

void MST_ResetComponentMinEdges(Edge *componentMinEdges, unsigned int nodeCount);
void MST_ComputeComponentMinEdges(Edge *edgeList, unsigned int *edgeCount, Edge *componentMinEdges,
                                  DisjointSetPool *disjointSetPool);
void MST_ApplyComponentMinEdges(Edge *componentMinEdges, DisjointSetPool *disjointSetPool, unsigned int nodeCount,
                                Edge *mstEdgeList, unsigned int *mstCount);

bool MST_Compute(Edge *edgeList, unsigned int *edgeCount, unsigned int nodeCount, Edge *mstEdgeList,
                 unsigned int *mstCount);

bool MST_ComputeParallel(Edge *edgeList, unsigned int *edgeCount, unsigned int nodeCount, Edge *mstEdgeList,
                         unsigned int *mstCount, MPI_Comm comm);

#endif
