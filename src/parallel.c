#include <limits.h>
#include <mpi.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "core/disjointset.h"
#include "core/graph.h"
#include "mpi/types.h"

void reduceMinEdge(void *in, void *out, int *length, MPI_Datatype *datatype) {
  Edge *inEdgeList = (Edge *)in;
  Edge *outEdgeList = (Edge *)out;
  unsigned int nodeCount = (unsigned int)*length;

  for (NodeId root = 0; root < nodeCount; root++) {
    outEdgeList[root] = Edge_MinEdge(inEdgeList[root], outEdgeList[root]);
  }
}

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

void ApplyGlobalComponentMinEdges(Edge *componentMinEdges, DisjointSetPool *components, Edge *outMST,
                                  unsigned int *mstCount, const unsigned int globalNodeCount) {
  for (NodeId root = 0; root < globalNodeCount; root++) {
    const Edge edge = componentMinEdges[root];

    if (edge.weight == NOTHING.weight)
      continue;

    if (DisjointSet_Union(components, edge.source, edge.destination)) {
      outMST[(*mstCount)++] = edge;
    }
  }
}

void ComputeMST(Edge *edgeList, unsigned int *edgeCount, const unsigned int globalNodeCount, Edge *outMST,
                unsigned int *mstCount, int worldRank) {
  DisjointSetPool components;
  DisjointSet_Initialize(&components, globalNodeCount);

  Edge *componentMinEdges = malloc(sizeof(Edge) * globalNodeCount);
  if (!componentMinEdges) {
    fprintf(stderr, "Error: failed to allocate componentMinEdges\n");
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }

  MPI_Op reduceMinEdgeOp;
  MPI_Op_create(reduceMinEdge, 1, &reduceMinEdgeOp);

  while (components.componentsCount > 1) {
    ResetComponentMinEdges(componentMinEdges, globalNodeCount);

    ComputeComponentMinEdges(edgeList, edgeCount, componentMinEdges, &components);
    MPI_Allreduce(MPI_IN_PLACE, componentMinEdges, (int)globalNodeCount, MPI_EDGE_TYPE, reduceMinEdgeOp,
                  MPI_COMM_WORLD);

    if (worldRank == 0) {
      ApplyGlobalComponentMinEdges(componentMinEdges, &components, outMST, mstCount, globalNodeCount);
    }

    MPI_Bcast(&components.componentsCount, 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
    MPI_Bcast(components.parents, globalNodeCount, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
    MPI_Bcast(components.ranks, globalNodeCount, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
  }

  MPI_Op_free(&reduceMinEdgeOp);
  DisjointSet_Finalize(&components);
  free(componentMinEdges);
}

int main(int argc, char **argv) {
  char *filename = argv[1];
  unsigned int nodeCount = atoi(argv[2]);

  MPI_Init(&argc, &argv);

  MPI_Register_Datatypes();

  int worldRank, worldSize;
  MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);
  MPI_Comm_size(MPI_COMM_WORLD, &worldSize);

  MPI_File file;

  MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_RDONLY, MPI_INFO_NULL, &file);

  MPI_Offset totalFileSizeBytes;
  MPI_File_get_size(file, &totalFileSizeBytes);
  unsigned int globalEdgeCount = totalFileSizeBytes / sizeof(Edge);

  unsigned int localEdgeCount = globalEdgeCount / worldSize;

  MPI_Offset offset = (MPI_Offset)worldRank * localEdgeCount * sizeof(Edge);

  if (worldRank == worldSize - 1) {
    localEdgeCount += (globalEdgeCount % worldSize);
  }

  Edge *edgeList = malloc(sizeof(Edge) * localEdgeCount);
  if (!edgeList) {
    fprintf(stderr, "Error: failed to allocate edgeList\n");
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }

  MPI_File_read_at(file, offset, edgeList, localEdgeCount * sizeof(Edge), MPI_BYTE, MPI_STATUS_IGNORE);

  MPI_File_close(&file);

  Edge *outMST = malloc(sizeof(Edge) * (nodeCount - 1));
  if (!outMST) {
    fprintf(stderr, "Error: failed to allocate outMST\n");
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }

  unsigned int mstCount = 0;

  MPI_Barrier(MPI_COMM_WORLD);

  double startClock = MPI_Wtime();

  ComputeMST(edgeList, &localEdgeCount, nodeCount, outMST, &mstCount, worldRank);

  double endClock = MPI_Wtime();
  double cpuTimeUsed = endClock - startClock;
  if (worldRank == 0) {
    long double sum = 0;
    printf("===== MST Computed (Parallel) =====\n");
    for (unsigned int i = 0; i < mstCount; i++) {
      sum += outMST[i].weight;
    }

    printf("Total MST Edge Count: %u\n", mstCount);
    printf("Expected Target Count: %u\n", nodeCount - 1);
    printf("Sum: %Lf\n", sum);
    printf("Execution Time: %f seconds\n", cpuTimeUsed);
    printf("Time resolution: %e\n", MPI_Wtick());
  }

  free(edgeList);
  free(outMST);
  MPI_Free_Datatypes();
  MPI_Finalize();
  return 0;
}
