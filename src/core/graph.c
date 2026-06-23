#include "graph.h"

Edge Edge_MinEdge(const Edge firstEdge, const Edge secondEdge) {
  if (firstEdge.weight < secondEdge.weight)
    return firstEdge;
  if (firstEdge.weight == secondEdge.weight && firstEdge.source < secondEdge.source)
    return firstEdge;
  return secondEdge;
}
