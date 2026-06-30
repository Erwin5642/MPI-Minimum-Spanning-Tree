#ifndef MST_OUTPUT_H
#define MST_OUTPUT_H

#include <stdbool.h>

#include "graph.h"

bool MST_Output_WriteToTxt(const char *outputPath, const Edge *mstEdgeList, unsigned int mstCount);

#endif
