#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mst.h"

#define BUFFER_SIZE_MB 32
#define BUFFER_EDGES ((BUFFER_SIZE_MB * 1024 * 1024) / sizeof(Edge))

#define MST_TEMP_PATH_A ".tmp/mst-a"
#define MST_TEMP_PATH_B ".tmp/mst-b"

void MST_ResetComponentMinEdges(Edge *componentMinEdges, unsigned int nodeCount) {
  for (NodeId rootNodeId = 0; rootNodeId < nodeCount; rootNodeId++) {
    componentMinEdges[rootNodeId] = GRAPH_NOTHING;
  }
}

void MST_ComputeComponentMinEdges(FILE *readFile, FILE *writeFile, Edge *readBuffer, Edge *writeBuffer,
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

      if (sourceRootId == destRootId)
        continue;

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

void MST_ApplyComponentMinEdges(Edge *componentMinEdges, DisjointSetPool *disjointSetPool, unsigned int nodeCount,
                                Edge *mstEdgeList, unsigned int *mstCount) {
  for (NodeId rootNodeId = 0; rootNodeId < nodeCount; rootNodeId++) {
    const Edge edge = componentMinEdges[rootNodeId];

    if (edge.weight == GRAPH_NOTHING.weight)
      continue;

    if (DisjointSet_Union(disjointSetPool, edge.source, edge.destination, edge.weight)) {
      mstEdgeList[(*mstCount)++] = edge;
    }
  }
}

void MST_Compute(const char *graphFilePath, unsigned int edgeCount, unsigned int nodeCount, Edge *mstEdgeList,
                 unsigned int *mstCount, Weight *mstWeightSum) {
    *mstWeightSum = 0;


  if (mkdir(".tmp", 0755) != 0 && errno != EEXIST) {
    fprintf(stderr, "Erro: MST: falha ao criar diretorio .tmp: %s\n", strerror(errno));
    return;
  }

  DisjointSetPool disjointSetPool;
  if (!DisjointSet_Initialize(&disjointSetPool, nodeCount)) {
    fprintf(stderr, "Erro: MST: falha ao inicializar conjunto disjunto\n");
    return;
  }

  Edge *componentMinEdges = malloc(sizeof(Edge) * nodeCount);
  Edge *readBuffer = malloc(sizeof(Edge) * BUFFER_EDGES);
  Edge *writeBuffer = malloc(sizeof(Edge) * BUFFER_EDGES);

  if (!componentMinEdges || !readBuffer || !writeBuffer) {
    fprintf(stderr, "Erro: MST: falha ao alocar buffers principais: %s\n", strerror(errno));
    if (componentMinEdges) free(componentMinEdges);
    if (readBuffer) free(readBuffer);
    if (writeBuffer) free(writeBuffer);
    DisjointSet_Finalize(&disjointSetPool);
    return;
  }

  const char *currentReadPath = graphFilePath;
  const char *currentWritePath = MST_TEMP_PATH_A;
  unsigned int currentEdgeCount = edgeCount;
  int phase = 0;
  
  while (disjointSetPool.componentCount > 1 && currentEdgeCount > 0) {
    FILE *readFile = fopen(currentReadPath, "rb");
    FILE *writeFile = fopen(currentWritePath, "wb");

    if (!readFile || !writeFile) {
      fprintf(stderr, "Erro: MST: falha ao abrir arquivos I/O (Fase %d): %s\n", phase, strerror(errno));
      if (readFile) fclose(readFile);
      if (writeFile) fclose(writeFile);
      break;
    }

    MST_ResetComponentMinEdges(componentMinEdges, nodeCount);

    MST_ComputeComponentMinEdges(readFile, writeFile, readBuffer, writeBuffer, &currentEdgeCount, componentMinEdges,
                                 &disjointSetPool);

    fclose(readFile);
    fclose(writeFile);

    MST_ApplyComponentMinEdges(componentMinEdges, &disjointSetPool, nodeCount, mstEdgeList, mstCount);

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

  *mstWeightSum = disjointSetPool.componentSums[disjointSetPool.lastUnifiedRootId];

  DisjointSet_Finalize(&disjointSetPool);
  free(readBuffer);
  free(writeBuffer);
  free(componentMinEdges);
}
