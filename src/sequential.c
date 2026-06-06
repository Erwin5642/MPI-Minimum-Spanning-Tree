#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "core/disjointset.h"
#include "core/graph.h"
#include "io/stream.h"

void ResetComponentMinEdges(Edge *componentMinEdges, const unsigned int nodeCount) {
  for (NodeId root = 0; root < nodeCount; root++) {
    componentMinEdges[root] = NOTHING;
  }
}

void ComputeComponentMinEdges(Edge *edgeList, unsigned int *edgeCount, Edge *componentMinEdges,
                              DisjointSetPool *components) {
  unsigned int updatedEdgeCount = 0;

  for (EdgeId edgeId = 0; edgeId < *edgeCount; edgeId++) {
    const Edge edge = edgeList[edgeId];

    const NodeId rootU = DisjointSet_Find(components, edge.source);
    const NodeId rootV = DisjointSet_Find(components, edge.destination);

    if (rootU == rootV)
      continue;

    edgeList[updatedEdgeCount++] = edge;

    Edge *componentMinEdgeOnU = &componentMinEdges[rootU];
    Edge *componentMinEdgeOnV = &componentMinEdges[rootV];

    if (edge.weight < componentMinEdgeOnU->weight) {
      *componentMinEdgeOnU = edge;
    }
    if (edge.weight < componentMinEdgeOnV->weight) {
      *componentMinEdgeOnV = edge;
    }
  }
  *edgeCount = updatedEdgeCount;
}

void ApplyComponentMinEdges(Edge *componentMinEdges, DisjointSetPool *components, Edge *mstOut, unsigned int *mstCount,
                            const unsigned int nodeCount) {
  for (NodeId root = 0; root < nodeCount; root++) {
    const Edge edge = componentMinEdges[root];

    if (edge.weight == NOTHING.weight)
      continue;

    if (DisjointSet_Union(components, edge.source, edge.destination)) {
      mstOut[(*mstCount)++] = edge;
    }
  }
}

void ComputeMST(Edge *edgeList, const unsigned int nodeCount, unsigned int *edgeCount, Edge *mstOut,
                unsigned int *mstCount) {
  DisjointSetPool components;
  if (!DisjointSet_Initialize(&components, nodeCount)) {
    fprintf(stderr, "Error: failed to allocate disjoint set: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  Edge *componentMinEdges = malloc(sizeof(Edge) * nodeCount);
  if (!componentMinEdges) {
    fprintf(stderr, "Error: failed to allocate component min edges: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  while (components.componentsCount > 1) {
    ResetComponentMinEdges(componentMinEdges, nodeCount);
    ComputeComponentMinEdges(edgeList, edgeCount, componentMinEdges, &components);
    ApplyComponentMinEdges(componentMinEdges, &components, mstOut, mstCount, nodeCount);
  }

  DisjointSet_Finalize(&components);
  free(componentMinEdges);
}

int main(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr, "Usage: %s <graphfile> <nodeCount>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  char *filename = argv[1];
  unsigned int nodeCount = atoi(argv[2]);

  size_t fileSizeBytes;
  Edge *edgeList = Stream_Map(filename, &fileSizeBytes);
  unsigned int edgeCount = (unsigned int)(fileSizeBytes / sizeof(Edge));

  Edge *mstOut = malloc(sizeof(Edge) * (nodeCount - 1));
  if (!mstOut) {
    fprintf(stderr, "Error: failed to allocate mst: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  unsigned int mstCount = 0;

  clock_t startClock = clock();

  ComputeMST(edgeList, nodeCount, &edgeCount, mstOut, &mstCount);

  clock_t endClock = clock();
  double cpuTimeUsed = ((double)(endClock - startClock)) / CLOCKS_PER_SEC;

  long double sum = 0;
  printf("===== MST Computed (Sequential) =====\n");
  for (unsigned int i = 0; i < mstCount; i++) {
    sum += mstOut[i].weight;
  }

  printf("Total MST Edge Count: %u\n", mstCount);
  printf("Expected Target Count: %u\n", nodeCount - 1);
  printf("Sum: %Lf\n", sum);
  printf("CPU Execution Time: %f seconds\n", cpuTimeUsed);

  free(mstOut);
  Stream_Unmap(edgeList, fileSizeBytes);

  return 0;
}
