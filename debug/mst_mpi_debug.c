#include <errno.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/core/disjointset.h"
#include "../src/mpi/types.h"
#include "mst_mpi_debug.h"

static void MST_Debug_ResetComponentMinEdgesParallel(Edge *componentMinEdges, unsigned int nodeCount) {
  for (NodeId rootNodeId = 0; rootNodeId < nodeCount; rootNodeId++) {
    componentMinEdges[rootNodeId] = GRAPH_NOTHING;
  }
}

static void MST_Debug_ComputeComponentMinEdgesParallel(Edge *edgeList, unsigned int *edgeCount, Edge *componentMinEdges,
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

static unsigned int MST_Debug_ApplyComponentMinEdgesParallel(Edge *componentMinEdges, DisjointSetPool *disjointSetPool,
                                                             unsigned int nodeCount, Edge *mstEdgeList,
                                                             unsigned int *mstCount) {
  unsigned int unionCount = 0;

  for (NodeId rootNodeId = 0; rootNodeId < nodeCount; rootNodeId++) {
    const Edge edge = componentMinEdges[rootNodeId];

    if (edge.weight == GRAPH_NOTHING.weight) {
      continue;
    }

    if (DisjointSet_Union(disjointSetPool, edge.source, edge.destination, edge.weight)) {
      mstEdgeList[(*mstCount)++] = edge;
      unionCount++;
    }
  }

  return unionCount;
}

void MST_ComputeParallelDebug(Edge *edgeList, unsigned int *edgeCount, unsigned int nodeCount, Edge *mstEdgeList,
                              unsigned int *mstCount, Weight *mstWeightSum) {
  if (mstWeightSum) {
    *mstWeightSum = 0;
  }

  int mpiRank;
  int mpiWorldSize;
  MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpiWorldSize);

  DisjointSetPool disjointSetPool;
  if (!DisjointSet_Initialize(&disjointSetPool, nodeCount)) {
    fprintf(stderr, "Erro: MST Debug MPI: falha ao inicializar conjunto disjunto\n");
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }

  Edge *componentMinEdges = malloc(sizeof(Edge) * nodeCount);
  if (!componentMinEdges) {
    fprintf(stderr, "Erro: MST Debug MPI: falha ao alocar arestas mínimas por componente: %s\n", strerror(errno));
    DisjointSet_Finalize(&disjointSetPool);
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }

  ComponentEdge *localValidEdges = malloc(sizeof(ComponentEdge) * nodeCount);
  ComponentEdge *globalValidEdges = malloc(sizeof(ComponentEdge) * nodeCount * mpiWorldSize);
  if (!localValidEdges || !globalValidEdges) {
    fprintf(stderr, "Erro: MST Debug MPI: falha ao alocar arestas mínimas por componente: %s\n", strerror(errno));
    free(componentMinEdges);
    free(localValidEdges);
    free(globalValidEdges);
    DisjointSet_Finalize(&disjointSetPool);
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }

  int *recvCounts = malloc(sizeof(int) * mpiWorldSize);
  int *displs = malloc(sizeof(int) * mpiWorldSize);
  if (!recvCounts || !displs) {
    fprintf(stderr, "Erro: MST Debug MPI: falha ao alocar vetores de comunicação: %s\n", strerror(errno));
    free(componentMinEdges);
    free(localValidEdges);
    free(globalValidEdges);
    free(recvCounts);
    free(displs);
    DisjointSet_Finalize(&disjointSetPool);
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }

  unsigned int phaseId = 0;
  // TODO(debug-remove): remover log de início de execução.
  printf("[progresso][paralelo-debug][rank=%d] Início da MST: componentes=%u arestasLocais=%u ranks=%d\n", mpiRank,
         disjointSetPool.componentCount, *edgeCount, mpiWorldSize);

  while (disjointSetPool.componentCount > 1) {
    unsigned int componentCountBefore = disjointSetPool.componentCount;
    unsigned int localEdgeCountBefore = *edgeCount;

    MST_Debug_ResetComponentMinEdgesParallel(componentMinEdges, nodeCount);
    MST_Debug_ComputeComponentMinEdgesParallel(edgeList, edgeCount, componentMinEdges, &disjointSetPool);

    int localValidCount = 0;
    for (NodeId componentId = 0; componentId < nodeCount; componentId++) {
      const Edge edge = componentMinEdges[componentId];
      if (edge.weight != GRAPH_NOTHING.weight) {
        localValidEdges[localValidCount++] = (ComponentEdge){componentId, edge};
      }
    }

    MPI_Allgather(&localValidCount, 1, MPI_INT, recvCounts, 1, MPI_INT, MPI_COMM_WORLD);

    int totalValidCount = 0;
    for (int rank = 0; rank < mpiWorldSize; rank++) {
      displs[rank] = totalValidCount;
      totalValidCount += recvCounts[rank];
    }

    // TODO(debug-remove): remover log de métricas de fase.
    printf("[progresso][paralelo-debug][rank=%d][fase=%u] componentes=%u arestasLocais=%u validasLocais=%d totalValidas=%d\n",
           mpiRank, phaseId, componentCountBefore, localEdgeCountBefore, localValidCount, totalValidCount);

    MPI_Allgatherv(localValidEdges, localValidCount, MPI_COMPONENT_EDGE_TYPE, globalValidEdges, recvCounts, displs,
                   MPI_COMPONENT_EDGE_TYPE, MPI_COMM_WORLD);

    MST_Debug_ResetComponentMinEdgesParallel(componentMinEdges, nodeCount);
    for (int i = 0; i < totalValidCount; i++) {
      NodeId componentId = globalValidEdges[i].component;
      Edge edge = globalValidEdges[i].edge;

      if (edge.weight < componentMinEdges[componentId].weight) {
        componentMinEdges[componentId] = edge;
      }
    }

    unsigned int localUnionCount = MST_Debug_ApplyComponentMinEdgesParallel(componentMinEdges, &disjointSetPool,
                                                                             nodeCount, mstEdgeList, mstCount);
    unsigned int componentCountAfter = disjointSetPool.componentCount;

    // TODO(debug-remove): remover log de uniões por fase.
    printf("[progresso][paralelo-debug][rank=%d][fase=%u] unioesAplicadas=%u componentesApos=%u mstCount=%u\n",
           mpiRank, phaseId, localUnionCount, componentCountAfter, *mstCount);

    if (componentCountAfter == componentCountBefore && componentCountAfter > 1) {
      // TODO(debug-remove): remover log de assinatura de estagnação.
      printf("[progresso][paralelo-debug][rank=%d][fase=%u] ALERTA: sem progresso nesta fase (componentes=%u)\n",
             mpiRank, phaseId, componentCountAfter);
    }
    if (totalValidCount == 0 && componentCountAfter > 1) {
      // TODO(debug-remove): remover log de ausência de arestas válidas.
      printf("[progresso][paralelo-debug][rank=%d][fase=%u] ALERTA: nenhuma aresta válida encontrada\n", mpiRank,
             phaseId);
    }

    phaseId++;
  }

  // TODO(debug-remove): remover log de finalização.
  printf("[progresso][paralelo-debug][rank=%d] Finalizado: fases=%u componentes=%u arestasLocaisFinais=%u mstCount=%u\n",
         mpiRank, phaseId, disjointSetPool.componentCount, *edgeCount, *mstCount);

  if (mstWeightSum && disjointSetPool.lastUnifiedRootId != GRAPH_NODE_ID_MAX) {
    *mstWeightSum = disjointSetPool.componentSums[disjointSetPool.lastUnifiedRootId];
  }

  free(displs);
  free(recvCounts);
  free(globalValidEdges);
  free(localValidEdges);
  DisjointSet_Finalize(&disjointSetPool);
  free(componentMinEdges);
}
