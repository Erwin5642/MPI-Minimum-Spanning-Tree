#ifndef GRAPH_H

#define GRAPH_H

#include <float.h>
#include <limits.h>

typedef unsigned int NodeId;
typedef unsigned int EdgeId;
typedef double Weight;

typedef struct {
  NodeId source;
  NodeId destination;
  Weight weight;
} Edge;

#define NOTHING ((Edge){UINT_MAX, UINT_MAX, DBL_MAX})

Edge Edge_MinEdge(const Edge first, const Edge second);

#endif
