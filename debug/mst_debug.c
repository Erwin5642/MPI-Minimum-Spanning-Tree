#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../src/mst/mst.h"
#include "mst_debug.h"

#define BUFFER_SIZE_MB 32
#define BUFFER_EDGES ((BUFFER_SIZE_MB * 1024 * 1024) / sizeof(Edge))

#define MST_TEMP_PATH_A ".tmp/mst-debug-a"
#define MST_TEMP_PATH_B ".tmp/mst-debug-b"

static void MST_Debug_ResetComponentMinEdges(Edge *componentMinEdges, unsigned int nodeCount) {
  for (NodeId rootNodeId = 0; rootNodeId < nodeCount; rootNodeId++) {
    componentMinEdges[rootNodeId] = GRAPH_NOTHING;
  }
}

static void MST_Debug_ComputeComponentMinEdges(FILE *readFile, FILE *writeFile, Edge *readBuffer, Edge *writeBuffer,
                                               unsigned int *edgeCount, Edge *componentMinEdges,
                                               DisjointSetPool *disjointSetPool) {
  unsigned int updatedEdgeCount = 0;
  unsigned int writeBufferCount = 0;

  unsigned int readCount;
  while ((readCount = fread(readBuffer, sizeof(Edge), BUFFER_EDGES, readFile)) > 0) {
    for (unsigned int edgeId = 0; edgeId < readCount; edgeId++) {
      const Edge edge = readBuffer[edgeId];

      const NodeId sourceRootId = DisjointSet_Find(disjointSetPool, edge.source);
      const NodeId destRootId = DisjointSet_Find(disjointSetPool, edge.destination);

      if (sourceRootId == destRootId) {
        continue;
      }

      writeBuffer[writeBufferCount++] = edge;
      updatedEdgeCount++;

      if (writeBufferCount == BUFFER_EDGES) {
        fwrite(writeBuffer, sizeof(Edge), writeBufferCount, writeFile);
        writeBufferCount = 0;
      }

      Edge *componentMinEdgeOnSource = &componentMinEdges[sourceRootId];
      Edge *componentMinEdgeOnDest = &componentMinEdges[destRootId];

      if (edge.weight < componentMinEdgeOnSource->weight) {
        *componentMinEdgeOnSource = edge;
      }
      if (edge.weight < componentMinEdgeOnDest->weight) {
        *componentMinEdgeOnDest = edge;
      }
    }
  }

  if (writeBufferCount > 0) {
    fwrite(writeBuffer, sizeof(Edge), writeBufferCount, writeFile);
  }

  *edgeCount = updatedEdgeCount;
}

static unsigned int MST_Debug_ApplyComponentMinEdges(Edge *componentMinEdges, DisjointSetPool *disjointSetPool,
                                                     unsigned int nodeCount, Edge *mstEdgeList, unsigned int *mstCount) {
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

void MST_ComputeDebug(const char *graphFilePath, unsigned int edgeCount, unsigned int nodeCount, Edge *mstEdgeList,
                      unsigned int *mstCount, Weight *mstWeightSum) {
  if (mstWeightSum) {
    *mstWeightSum = 0;
  }

  if (mkdir(".tmp", 0755) != 0 && errno != EEXIST) {
    fprintf(stderr, "Erro: MST Debug: falha ao criar diretório .tmp: %s\n", strerror(errno));
    return;
  }

  DisjointSetPool disjointSetPool;
  if (!DisjointSet_Initialize(&disjointSetPool, nodeCount)) {
    fprintf(stderr, "Erro: MST Debug: falha ao inicializar conjunto disjunto\n");
    return;
  }

  Edge *componentMinEdges = malloc(sizeof(Edge) * nodeCount);
  Edge *readBuffer = malloc(sizeof(Edge) * BUFFER_EDGES);
  Edge *writeBuffer = malloc(sizeof(Edge) * BUFFER_EDGES);

  if (!componentMinEdges || !readBuffer || !writeBuffer) {
    fprintf(stderr, "Erro: MST Debug: falha ao alocar buffers principais: %s\n", strerror(errno));
    free(componentMinEdges);
    free(readBuffer);
    free(writeBuffer);
    DisjointSet_Finalize(&disjointSetPool);
    return;
  }

  const char *currentReadPath = graphFilePath;
  const char *currentWritePath = MST_TEMP_PATH_A;
  unsigned int currentEdgeCount = edgeCount;
  unsigned int phase = 0;

  while (disjointSetPool.componentCount > 1 && currentEdgeCount > 0) {
    // TODO(debug-remove): remover log de início de fase.
    printf("[progresso][sequencial-debug] Iniciando fase %u: componentes=%u arestas=%u\n", phase,
           disjointSetPool.componentCount, currentEdgeCount);

    FILE *readFile = fopen(currentReadPath, "rb");
    FILE *writeFile = fopen(currentWritePath, "wb");

    if (!readFile || !writeFile) {
      fprintf(stderr, "Erro: MST Debug: falha ao abrir arquivos de I/O na fase %u: %s\n", phase, strerror(errno));
      if (readFile) {
        fclose(readFile);
      }
      if (writeFile) {
        fclose(writeFile);
      }
      break;
    }

    MST_Debug_ResetComponentMinEdges(componentMinEdges, nodeCount);
    MST_Debug_ComputeComponentMinEdges(readFile, writeFile, readBuffer, writeBuffer, &currentEdgeCount,
                                       componentMinEdges, &disjointSetPool);

    fclose(readFile);
    fclose(writeFile);

    // TODO(debug-remove): remover log após filtragem.
    printf("[progresso][sequencial-debug] Fase %u: arestas após filtragem=%u\n", phase, currentEdgeCount);

    unsigned int unionsApplied = MST_Debug_ApplyComponentMinEdges(componentMinEdges, &disjointSetPool, nodeCount,
                                                                  mstEdgeList, mstCount);

    // TODO(debug-remove): remover log após uniões.
    printf("[progresso][sequencial-debug] Fase %u: unioes aplicadas=%u componentes=%u mstCount=%u\n", phase,
           unionsApplied, disjointSetPool.componentCount, *mstCount);

    if (phase == 0) {
      currentReadPath = MST_TEMP_PATH_A;
      currentWritePath = MST_TEMP_PATH_B;
    } else {
      const char *nextReadPath = currentWritePath;
      currentWritePath = currentReadPath;
      currentReadPath = nextReadPath;
    }

    phase++;
  }

  if (disjointSetPool.componentCount > 1 && currentEdgeCount == 0) {
    // TODO(debug-remove): remover log de estagnação.
    printf("[progresso][sequencial-debug] Encerrado sem progresso: componentes=%u e arestas=0\n",
           disjointSetPool.componentCount);
  }

  // TODO(debug-remove): remover log de finalização.
  printf("[progresso][sequencial-debug] Finalizado: fases=%u componentes=%u mstCount=%u\n", phase,
         disjointSetPool.componentCount, *mstCount);

  if (mstWeightSum && disjointSetPool.lastUnifiedRootId != GRAPH_NODE_ID_MAX) {
    *mstWeightSum = disjointSetPool.componentSums[disjointSetPool.lastUnifiedRootId];
  }

  DisjointSet_Finalize(&disjointSetPool);
  free(readBuffer);
  free(writeBuffer);
  free(componentMinEdges);
}
