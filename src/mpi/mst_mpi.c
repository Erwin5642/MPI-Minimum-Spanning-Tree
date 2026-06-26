#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../core/disjointset.h"
#include "mpi.h"
#include "mst_mpi.h"
#include "types.h"

void MST_ResetComponentMinEdgesParallel(Edge *componentMinEdges, unsigned int nodeCount) {
  for (NodeId rootNodeId = 0; rootNodeId < nodeCount; rootNodeId++) {
    componentMinEdges[rootNodeId] = GRAPH_NOTHING;
  }
}

void MST_ComputeComponentMinEdgesParallel(Edge *edgeList, unsigned int *edgeCount, Edge *componentMinEdges,
                                          DisjointSetPool *disjointSetPool) {
  unsigned int updatedEdgeCount = 0;

  for (EdgeId edgeId = 0; edgeId < *edgeCount; edgeId++) {
    const Edge edge = edgeList[edgeId];

    const NodeId sourceRootId = DisjointSet_Find(disjointSetPool, edge.source);
    const NodeId destRootId = DisjointSet_Find(disjointSetPool, edge.destination);

    if (sourceRootId == destRootId)
      continue;

    edgeList[updatedEdgeCount++] = edge;

    Edge *componentMinEdgeOnSource = &componentMinEdges[sourceRootId];
    Edge *componentMinEdgeOnDest = &componentMinEdges[destRootId];

    if (edge.weight < componentMinEdgeOnSource->weight) {
      *componentMinEdgeOnSource = edge;
    }
    if (edge.weight < componentMinEdgeOnDest->weight) {
      *componentMinEdgeOnDest = edge;
    }
  }
  *edgeCount = updatedEdgeCount;
}

unsigned int MST_ApplyComponentMinEdgesParallel(Edge *componentMinEdges, DisjointSetPool *disjointSetPool,
                                                unsigned int nodeCount, Edge *mstEdgeList, unsigned int *mstCount) {
  unsigned int unionCount = 0;
  for (NodeId rootNodeId = 0; rootNodeId < nodeCount; rootNodeId++) {
    const Edge edge = componentMinEdges[rootNodeId];

    if (edge.weight == GRAPH_NOTHING.weight)
      continue;

    if (DisjointSet_Union(disjointSetPool, edge.source, edge.destination)) {
      mstEdgeList[(*mstCount)++] = edge;
      unionCount++;
    }
  }
  return unionCount;
}

void MST_ReduceMinEdge(void *inBuffer, void *outBuffer, int *length, MPI_Datatype *datatype) {
  (void)datatype;
  Edge *inEdgeList = (Edge *)inBuffer;
  Edge *outEdgeList = (Edge *)outBuffer;
  unsigned int nodeCount = (unsigned int)*length;

  for (NodeId rootNodeId = 0; rootNodeId < nodeCount; rootNodeId++) {
    outEdgeList[rootNodeId] = Edge_MinEdge(inEdgeList[rootNodeId], outEdgeList[rootNodeId]);
  }
}

void MST_ComputeParallel(Edge *edgeList, unsigned int *edgeCount, unsigned int nodeCount, Edge *mstEdgeList,
                         unsigned int *mstCount) {
  int mpiRank, mpiWorldSize;
  MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpiWorldSize);
  const bool verboseRanks = getenv("MST_MPI_VERBOSE_RANKS") != NULL;
  const double mstComputeStartTime = MPI_Wtime();

  DisjointSetPool disjointSetPool;
  if (!DisjointSet_Initialize(&disjointSetPool, nodeCount)) {
    fprintf(stderr, "Erro: MST: falha ao inicializar conjunto disjunto\n");
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }

  Edge *componentMinEdges = malloc(sizeof(Edge) * nodeCount);
  if (!componentMinEdges) {
    fprintf(stderr, "Erro: MST: falha ao alocar arestas mínimas por componente: %s\n", strerror(errno));
    DisjointSet_Finalize(&disjointSetPool);
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }

  ComponentEdge *localValidEdges = malloc(sizeof(ComponentEdge) * nodeCount);
  ComponentEdge *globalValidEdges = malloc(sizeof(ComponentEdge) * nodeCount * mpiWorldSize);
  if (!localValidEdges || !globalValidEdges) {
    fprintf(stderr, "Erro: MST: falha ao alocar diff para arestas mínimas por componente: %s\n", strerror(errno));
    DisjointSet_Finalize(&disjointSetPool);
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }

  int *recvCounts = malloc(sizeof(int) * mpiWorldSize);
  int *displs = malloc(sizeof(int) * mpiWorldSize);

  MPI_Op reduceMinEdgeOp;
  MPI_Op_create(MST_ReduceMinEdge, 1, &reduceMinEdgeOp);

  unsigned int phaseId = 0;
  while (disjointSetPool.componentCount > 1) {
    const double phaseStartTime = MPI_Wtime();
    const unsigned int componentCountBefore = disjointSetPool.componentCount;
    const unsigned int localEdgeCountBefore = *edgeCount;

    const double computeMinStartTime = MPI_Wtime();
    MST_ResetComponentMinEdgesParallel(componentMinEdges, nodeCount);
    MST_ComputeComponentMinEdgesParallel(edgeList, edgeCount, componentMinEdges, &disjointSetPool);
    const double computeMinElapsedSeconds = MPI_Wtime() - computeMinStartTime;
  
    int localValidCount = 0;
    for (NodeId componentId = 0; componentId < nodeCount; componentId++) {
      const Edge edge = componentMinEdges[componentId];
      if (edge.weight != GRAPH_NOTHING.weight) {
        localValidEdges[localValidCount++] = (ComponentEdge){componentId, edge};
      }
    }

    const double gatherCountsStartTime = MPI_Wtime();
    MPI_Allgather(&localValidCount, 1, MPI_INT, recvCounts, 1, MPI_INT, MPI_COMM_WORLD);
    const double gatherCountsElapsedSeconds = MPI_Wtime() - gatherCountsStartTime;

    int totalValidCount = 0;
    for (int rank = 0; rank < mpiWorldSize; rank++) {
      displs[rank] = totalValidCount;
      totalValidCount += recvCounts[rank];
    }

    const double gatherEdgesStartTime = MPI_Wtime();
    MPI_Allgatherv(localValidEdges, localValidCount, MPI_COMPONENT_EDGE_TYPE, globalValidEdges, recvCounts, displs,
                   MPI_COMPONENT_EDGE_TYPE, MPI_COMM_WORLD);
    const double gatherEdgesElapsedSeconds = MPI_Wtime() - gatherEdgesStartTime;

    const double rebuildMinStartTime = MPI_Wtime();
    MST_ResetComponentMinEdgesParallel(componentMinEdges, nodeCount);
    for (int i = 0; i < totalValidCount; i++) {
      NodeId componentId = globalValidEdges[i].component;
      Edge edge = globalValidEdges[i].edge;

      if (edge.weight < componentMinEdges[componentId].weight) {
        componentMinEdges[componentId] = edge;
      }
    }
    const double rebuildMinElapsedSeconds = MPI_Wtime() - rebuildMinStartTime;
    
    const double applyMinStartTime = MPI_Wtime();
    const unsigned int localUnionCount =
        MST_ApplyComponentMinEdgesParallel(componentMinEdges, &disjointSetPool, nodeCount, mstEdgeList, mstCount);
    const double applyMinElapsedSeconds = MPI_Wtime() - applyMinStartTime;
    const unsigned int componentCountAfter = disjointSetPool.componentCount;
    const unsigned int localEdgeCountAfter = *edgeCount;

    unsigned int minUnionCount = localUnionCount;
    unsigned int maxUnionCount = localUnionCount;
    const double unionReduceStartTime = MPI_Wtime();
    MPI_Allreduce(MPI_IN_PLACE, &minUnionCount, 1, MPI_UNSIGNED, MPI_MIN, MPI_COMM_WORLD);
    MPI_Allreduce(MPI_IN_PLACE, &maxUnionCount, 1, MPI_UNSIGNED, MPI_MAX, MPI_COMM_WORLD);
    const double unionReduceElapsedSeconds = MPI_Wtime() - unionReduceStartTime;
    const double phaseElapsedSeconds = MPI_Wtime() - phaseStartTime;

    if (verboseRanks) {
      fprintf(stderr,
              "[rank=%d phase=%u] components=%u->%u localEdges=%u->%u localValid=%d totalValid=%d unions=%u "
              "timing[s]: phase=%.6f computeMin=%.6f gatherCounts=%.6f gatherEdges=%.6f rebuildMin=%.6f "
              "apply=%.6f unionReduce=%.6f\n",
              mpiRank, phaseId, componentCountBefore, componentCountAfter, localEdgeCountBefore, localEdgeCountAfter,
              localValidCount, totalValidCount, localUnionCount, phaseElapsedSeconds, computeMinElapsedSeconds,
              gatherCountsElapsedSeconds, gatherEdgesElapsedSeconds, rebuildMinElapsedSeconds, applyMinElapsedSeconds,
              unionReduceElapsedSeconds);
    }

    if (mpiRank == 0) {
      fprintf(stderr,
              "[phase=%u] components=%u->%u totalValid=%d unions=%u localEdges(rank0)=%u->%u timing[s]: phase=%.6f "
              "computeMin=%.6f gatherCounts=%.6f gatherEdges=%.6f rebuildMin=%.6f apply=%.6f unionReduce=%.6f\n",
              phaseId, componentCountBefore, componentCountAfter, totalValidCount, localUnionCount, localEdgeCountBefore,
              localEdgeCountAfter, phaseElapsedSeconds, computeMinElapsedSeconds, gatherCountsElapsedSeconds,
              gatherEdgesElapsedSeconds, rebuildMinElapsedSeconds, applyMinElapsedSeconds, unionReduceElapsedSeconds);
      if (minUnionCount != maxUnionCount) {
        fprintf(stderr, "[phase=%u] warning: inconsistent unions across ranks: min=%u max=%u\n", phaseId,
                minUnionCount, maxUnionCount);
      }
      if ((totalValidCount == 0 || localUnionCount == 0) && componentCountAfter > 1) {
        fprintf(stderr,
                "[phase=%u] stall-signature: components=%u totalValid=%d unions=%u localEdges(rank0)=%u\n", phaseId,
                componentCountAfter, totalValidCount, localUnionCount, localEdgeCountAfter);
      }
    }

    phaseId++;
  }

  const double mstComputeElapsedSeconds = MPI_Wtime() - mstComputeStartTime;
  fprintf(stderr, "[rank=%d] mst-compute-loop: elapsed=%.6fs phases=%u componentsFinal=%u edgesFinal=%u mstCount=%u\n",
          mpiRank, mstComputeElapsedSeconds, phaseId, disjointSetPool.componentCount, *edgeCount, *mstCount);

  free(globalValidEdges);
  free(localValidEdges);
  MPI_Op_free(&reduceMinEdgeOp);
  DisjointSet_Finalize(&disjointSetPool);
  free(componentMinEdges);
}
