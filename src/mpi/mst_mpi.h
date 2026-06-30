#ifndef MST_MPI_H
#define MST_MPI_H

#include <mpi.h>
#include <stdbool.h>

#include "../core/graph.h"

void MST_ComputeParallel(Edge *edgeList, unsigned int *edgeCount, unsigned int nodeCount, Edge *mstEdgeList,
                         unsigned int *mstCount, Weight *mstWeightSum);

#endif
