#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../core/disjointset.h"
#include "mpi.h"
#include "mst_mpi_all_reduce.h"
#include "types.h"

static void MST_ResetComponentMinEdgesParallel(Edge *componentMinEdges, unsigned int nodeCount) {
  for (NodeId rootNodeId = 0; rootNodeId < nodeCount; rootNodeId++) {
    componentMinEdges[rootNodeId] = GRAPH_NOTHING;
  }
}

static void MST_ComputeComponentMinEdgesParallel(Edge *edgeList, unsigned int *edgeCount, Edge *componentMinEdges,
                                                 DisjointSetPool *disjointSetPool) {
  unsigned int updatedEdgeCount = 0;

  for (EdgeId edgeId = 0; edgeId < *edgeCount; edgeId++) {
    const Edge edge = edgeList[edgeId];

    const NodeId sourceRootId = DisjointSet_Find(disjointSetPool, edge.source);
    const NodeId destRootId = DisjointSet_Find(disjointSetPool, edge.destination);

    if (sourceRootId == destRootId) {
      continue;
    }

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

static void MST_ApplyComponentMinEdgesParallel(Edge *componentMinEdges, DisjointSetPool *disjointSetPool,
                                               unsigned int nodeCount, Edge *mstEdgeList, unsigned int *mstCount) {
  for (NodeId rootNodeId = 0; rootNodeId < nodeCount; rootNodeId++) {
    const Edge edge = componentMinEdges[rootNodeId];

    if (edge.weight == GRAPH_NOTHING.weight) {
      continue;
    }

    if (DisjointSet_Union(disjointSetPool, edge.source, edge.destination, edge.weight)) {
      mstEdgeList[(*mstCount)++] = edge;
    }
  }
}

static void MST_ReduceMinEdge(void *inBuffer, void *outBuffer, int *length, MPI_Datatype *datatype) {
  (void)datatype;
  Edge *inEdgeList = (Edge *)inBuffer;
  Edge *outEdgeList = (Edge *)outBuffer;
  unsigned int nodeCount = (unsigned int)*length;

  for (NodeId rootNodeId = 0; rootNodeId < nodeCount; rootNodeId++) {
    outEdgeList[rootNodeId] = Edge_MinEdge(inEdgeList[rootNodeId], outEdgeList[rootNodeId]);
  }
}

void MST_ComputeParallelAllReduce(Edge *edgeList, unsigned int *edgeCount, unsigned int nodeCount, Edge *mstEdgeList,
                                  unsigned int *mstCount, Weight *mstWeightSum) {
  if (mstWeightSum) {
    *mstWeightSum = 0;
  }

  DisjointSetPool disjointSetPool;
  if (!DisjointSet_Initialize(&disjointSetPool, nodeCount)) {
    fprintf(stderr, "Erro: MST AllReduce: falha ao inicializar conjunto disjunto\n");
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }

  Edge *componentMinEdges = malloc(sizeof(Edge) * nodeCount);
  if (!componentMinEdges) {
    fprintf(stderr, "Erro: MST AllReduce: falha ao alocar arestas mínimas por componente: %s\n", strerror(errno));
    DisjointSet_Finalize(&disjointSetPool);
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }

  MPI_Op reduceMinEdgeOp;
  MPI_Op_create(MST_ReduceMinEdge, 1, &reduceMinEdgeOp);

  while (disjointSetPool.componentCount > 1) {
    MST_ResetComponentMinEdgesParallel(componentMinEdges, nodeCount);
    MST_ComputeComponentMinEdgesParallel(edgeList, edgeCount, componentMinEdges, &disjointSetPool);

    MPI_Allreduce(MPI_IN_PLACE, componentMinEdges, (int)nodeCount, MPI_EDGE_TYPE, reduceMinEdgeOp, MPI_COMM_WORLD);

    MST_ApplyComponentMinEdgesParallel(componentMinEdges, &disjointSetPool, nodeCount, mstEdgeList, mstCount);
  }

  if (mstWeightSum && disjointSetPool.lastUnifiedRootId != GRAPH_NODE_ID_MAX) {
    *mstWeightSum = disjointSetPool.componentSums[disjointSetPool.lastUnifiedRootId];
  }

  MPI_Op_free(&reduceMinEdgeOp);
  DisjointSet_Finalize(&disjointSetPool);
  free(componentMinEdges);
}
