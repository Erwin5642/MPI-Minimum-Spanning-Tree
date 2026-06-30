#ifndef MST_MPI_DEBUG_H
#define MST_MPI_DEBUG_H

#include "../src/core/graph.h"

void MST_ComputeParallelDebug(Edge *edgeList, unsigned int *edgeCount, unsigned int nodeCount, Edge *mstEdgeList,
                              unsigned int *mstCount, Weight *mstWeightSum);

#endif
