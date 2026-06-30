#ifndef MST_DEBUG_H
#define MST_DEBUG_H

#include "../src/core/graph.h"

void MST_ComputeDebug(const char *graphFilePath, unsigned int edgeCount, unsigned int nodeCount, Edge *mstEdgeList,
                      unsigned int *mstCount, Weight *mstWeightSum);

#endif
