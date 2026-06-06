#include "graph.h"

Edge Edge_MinEdge(const Edge first, const Edge second) {
  if (first.weight < second.weight)
    return first;
  if (first.weight == second.weight && first.source < second.source)
    return first;
  return second;
}
