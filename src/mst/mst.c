#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mst.h"

#define BUFFER_SIZE_MB 32
#define BUFFER_EDGES ((BUFFER_SIZE_MB * 1024 * 1024) / sizeof(Edge))

void MST_ResetComponentMinEdges(Edge *componentMinEdges, unsigned int nodeCount) {
  for (NodeId rootNodeId = 0; rootNodeId < nodeCount; rootNodeId++) {
    componentMinEdges[rootNodeId] = GRAPH_NOTHING;
  }
}

void MST_ComputeComponentMinEdges(FILE *readFile, FILE *writeFile, Edge *readBuffer, Edge *writeBuffer,
                                  unsigned int *edgeCount, Edge *componentMinEdges,
                                  DisjointSetPool *disjointSetPool) {
  unsigned int remainingEdges = *edgeCount;
  unsigned int updatedEdgeCount = 0;
  unsigned int writeBufferCount = 0;

  while (remainingEdges > 0) {
    unsigned int batchCount = remainingEdges < BUFFER_EDGES ? remainingEdges : BUFFER_EDGES;
    unsigned int readCount = fread(readBuffer, sizeof(Edge), batchCount, readFile);

    remainingEdges -= readCount;

    for (EdgeId edgeId = 0; edgeId < readCount; edgeId++) {
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

  fclose(readFile);
  fclose(writeFile);
}

void MST_ApplyComponentMinEdges(Edge *componentMinEdges, DisjointSetPool *disjointSetPool, unsigned int nodeCount,
                                Edge *mstEdgeList, unsigned int *mstCount) {
  for (NodeId rootNodeId = 0; rootNodeId < nodeCount; rootNodeId++) {
    const Edge edge = componentMinEdges[rootNodeId];

    if (edge.weight == GRAPH_NOTHING.weight)
      continue;

    if (DisjointSet_Union(disjointSetPool, edge.source, edge.destination)) {
      mstEdgeList[(*mstCount)++] = edge;
    }
  }
}

void MST_Compute(const char *graphFilePath, unsigned int edgeCount, unsigned int nodeCount, Edge *mstEdgeList,
                 unsigned int *mstCount) {
  char readPathA[] = "/tmp/mst-a-XXXXXX";
  char readPathB[] = "/tmp/mst-b-XXXXXX";
  int readFdA = mkstemp(readPathA);
  if (readFdA < 0) {
    fprintf(stderr, "Erro: MST: falha ao criar arquivo temporário A: %s\n", strerror(errno));
    return;
  }
  close(readFdA);

  int readFdB = mkstemp(readPathB);
  if (readFdB < 0) {
    fprintf(stderr, "Erro: MST: falha ao criar arquivo temporário B: %s\n", strerror(errno));
    unlink(readPathA);
    return;
  }
  close(readFdB);

  FILE *sourceFile = fopen(graphFilePath, "rb");
  FILE *readInitialFile = fopen(readPathA, "wb");
  if (!sourceFile || !readInitialFile) {
    fprintf(stderr, "Erro: MST: falha ao preparar arquivo temporário inicial: %s\n", strerror(errno));
    if (sourceFile)
      fclose(sourceFile);
    if (readInitialFile)
      fclose(readInitialFile);
    unlink(readPathA);
    unlink(readPathB);
    return;
  }

  Edge *copyBuffer = malloc(sizeof(Edge) * BUFFER_EDGES);
  if (!copyBuffer) {
    fprintf(stderr, "Erro: MST: falha ao alocar buffer de cópia inicial: %s\n", strerror(errno));
    fclose(sourceFile);
    fclose(readInitialFile);
    unlink(readPathA);
    unlink(readPathB);
    return;
  }

  unsigned int copiedEdges = 0;
  while (copiedEdges < edgeCount) {
    unsigned int copyCount = (edgeCount - copiedEdges) < BUFFER_EDGES ? (edgeCount - copiedEdges) : BUFFER_EDGES;
    unsigned int readCount = fread(copyBuffer, sizeof(Edge), copyCount, sourceFile);

    fwrite(copyBuffer, sizeof(Edge), readCount, readInitialFile);

    copiedEdges += readCount;
  }
  free(copyBuffer);

  fclose(sourceFile);
  fclose(readInitialFile);

  DisjointSetPool disjointSetPool;
  if (!DisjointSet_Initialize(&disjointSetPool, nodeCount)) {
    fprintf(stderr, "Erro: MST: falha ao inicializar conjunto disjunto\n");
    unlink(readPathA);
    unlink(readPathB);
    return;
  }

  Edge *componentMinEdges = malloc(sizeof(Edge) * nodeCount);
  if (!componentMinEdges) {
    fprintf(stderr, "Erro: MST: falha ao alocar arestas mínimas por componente: %s\n", strerror(errno));
    DisjointSet_Finalize(&disjointSetPool);
    unlink(readPathA);
    unlink(readPathB);
    return;
  }

  Edge *readBuffer = malloc(sizeof(Edge) * BUFFER_EDGES);
  Edge *writeBuffer = malloc(sizeof(Edge) * BUFFER_EDGES);
  if (!readBuffer || !writeBuffer) {
    fprintf(stderr, "Erro: MST: falha ao alocar buffers de leitura/escrita da MST: %s\n", strerror(errno));
    free(componentMinEdges);
    free(readBuffer);
    free(writeBuffer);
    DisjointSet_Finalize(&disjointSetPool);
    unlink(readPathA);
    unlink(readPathB);
    return;
  }

  const char *currentReadPath = readPathA;
  const char *currentWritePath = readPathB;
  unsigned int currentEdgeCount = edgeCount;

  while (disjointSetPool.componentCount > 1 && currentEdgeCount > 0) {
    FILE *readFile = fopen(currentReadPath, "rb");
    FILE *writeFile = fopen(currentWritePath, "wb");

    MST_ResetComponentMinEdges(componentMinEdges, nodeCount);

    MST_ComputeComponentMinEdges(readFile, writeFile, readBuffer, writeBuffer, &currentEdgeCount, componentMinEdges,
                                 &disjointSetPool);

    MST_ApplyComponentMinEdges(componentMinEdges, &disjointSetPool, nodeCount, mstEdgeList, mstCount);

    const char *nextReadPath = currentWritePath;
    currentWritePath = currentReadPath;
    currentReadPath = nextReadPath;
  }

  DisjointSet_Finalize(&disjointSetPool);
  free(readBuffer);
  free(writeBuffer);
  free(componentMinEdges);
  unlink(readPathA);
  unlink(readPathB);
}
