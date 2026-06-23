#ifndef GRAPH_H

#define GRAPH_H

#include <float.h>
#include <limits.h>

#define GRAPH_NODE_ID_MAX UINT_MAX
#define GRAPH_WEIGHT_MAX DBL_MAX

typedef unsigned int NodeId;
typedef unsigned int EdgeId;
typedef double Weight;

typedef struct {
  NodeId source;
  NodeId destination;
  Weight weight;
} Edge;

#define GRAPH_NOTHING ((Edge){GRAPH_NODE_ID_MAX, GRAPH_NODE_ID_MAX, GRAPH_WEIGHT_MAX})

Edge Edge_MinEdge(const Edge firstEdge, const Edge secondEdge);

#endif
