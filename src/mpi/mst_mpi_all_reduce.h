#ifndef MST_MPI_ALL_REDUCE_H
#define MST_MPI_ALL_REDUCE_H

#include "../core/graph.h"

void MST_ComputeParallelAllReduce(Edge *edgeList, unsigned int *edgeCount, unsigned int nodeCount, Edge *mstEdgeList,
                                  unsigned int *mstCount, Weight *mstWeightSum);

#endif
