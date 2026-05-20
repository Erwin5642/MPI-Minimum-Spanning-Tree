#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

/* Data Types */

typedef enum { False = 0, True = 1 } Bool;

typedef unsigned int NodeId;
typedef unsigned int EdgeId;
typedef double Weight;

typedef struct {
  NodeId source;
  NodeId destination;
  Weight weight;
} Edge;

#define NOTHING                                                                \
  (Edge) { 0, 0, UINT_MAX }

typedef struct {
  Edge *edges;
  unsigned int count;
  unsigned int capacity;
} EdgeChunk;

#define CHUNK_SIZE 4000000u

typedef struct {
  NodeId *parents;
  unsigned int *ranks;
  unsigned int nodeCount;
} DisjointSetPool;

/* Transformations */

void EdgeChunk_Initialize(EdgeChunk *chunk, const unsigned int capacity) {
  chunk->count = 0;
  chunk->capacity = capacity;
  chunk->edges = malloc(sizeof(Edge) * capacity);
}

void EdgeChunk_Finalize(EdgeChunk *chunk) {
  chunk->count = chunk->capacity = 0;
  free(chunk->edges);
  chunk->edges = NULL;
}

void DisjointSet_Initialize(DisjointSetPool *pool,
                            const unsigned int nodeCount) {
  pool->nodeCount = nodeCount;
  pool->ranks = calloc(nodeCount, sizeof(unsigned int));
  pool->parents = malloc(sizeof(NodeId) * nodeCount);

  NodeId *parents = pool->parents;

  for (unsigned int i = 0; i < nodeCount; i++) {
    parents[i] = i;
  }
}

void DisjointSet_Finalize(DisjointSetPool *pool) {
  pool->nodeCount = 0;
  free(pool->ranks);
  free(pool->parents);
  pool->parents = pool->ranks = NULL;
}

NodeId DisjointSet_Find(DisjointSetPool *pool, const NodeId target) {
  NodeId currentNode = target;
  NodeId *parent = pool->parents;

  while (currentNode != parent[currentNode]) {
    parent[currentNode] = parent[parent[currentNode]];
    currentNode = parent[currentNode];
  }

  return currentNode;
}

Bool DisjointSet_Union(DisjointSetPool *pool, const NodeId first,
                       const NodeId second) {
  NodeId rootFirst = DisjointSet_Find(pool, first);
  NodeId rootSecond = DisjointSet_Find(pool, second);

  if (rootFirst == rootSecond)
    return False;

  NodeId *parents = pool->parents;
  unsigned int *ranks = pool->ranks;

  if (ranks[rootFirst] < ranks[rootSecond]) {
    NodeId temp = rootFirst;
    rootFirst = rootSecond;
    rootSecond = temp;
  }

  parents[rootSecond] = rootFirst;

  if (ranks[rootFirst] == ranks[rootSecond]) {
    ranks[rootFirst]++;
  }

  return True;
}

FILE *Stream_Open(const char *filename, const char *mode) {
  FILE *file = fopen(filename, mode);

  if (!file) {
    fprintf(stderr, "Could not open the file: %s\n", filename);
    exit(EXIT_FAILURE);
  }

  return file;
}

void Stream_CopyFile(const char *sourcePath, const char *destPath) {
  FILE *src = Stream_Open(sourcePath, "rb");
  FILE *dst = Stream_Open(destPath, "wb");

  char buffer[16384];
  size_t bytesRead;

  while ((bytesRead = fread(buffer, 1, sizeof(buffer), src)) > 0) {
    fwrite(buffer, 1, bytesRead, dst);
  }

  fclose(src);
  fclose(dst);
}

void Stream_CopyTxtToBin(const char *sourcePath, const char *destPath) {
  FILE *binFile = Stream_Open(destPath, "wb");
  FILE *txtFile = Stream_Open(sourcePath, "r");

  unsigned int nodeCount, edgeCount;
  fscanf(txtFile, "%u %u", &nodeCount, &edgeCount);

  for (EdgeId i = 0; i < edgeCount; i++) {
    Edge temp;
    if (fscanf(txtFile, "%u %u %lf", &temp.source, &temp.destination,
               &temp.weight) == 3) {
      fwrite(&temp, sizeof(Edge), 1, binFile);
    }
  }

  fclose(txtFile);
  fclose(binFile);
}

void Stream_Initialize(const char *sourcePath, const char *destPath) {
  char *extension = strchr(sourcePath, '.');
  if (extension && strcmp(extension, ".txt") == 0) {
    Stream_CopyTxtToBin(sourcePath, destPath);
    return;
  }

  Stream_CopyFile(sourcePath, destPath);
}

void Stream_Finalize(FILE *file) {
  if (file)
    fclose(file);
}

void MST_ProcessMST(const char *filename, unsigned int nodeCount) {
  Stream_Initialize(filename, "graph_in_cpy.bin");

  DisjointSetPool components;
  DisjointSet_Initialize(&components, nodeCount);

  EdgeChunk readBuffer;
  EdgeChunk_Initialize(&readBuffer, CHUNK_SIZE);

  EdgeChunk auxReadBuffer;
  EdgeChunk_Initialize(&auxReadBuffer, CHUNK_SIZE);

  EdgeChunk mst;
  EdgeChunk_Initialize(&mst, nodeCount - 1);

  Edge *cheapestEdges = malloc(sizeof(Edge) * nodeCount);

  char *inFilename = "graph_in_cpy.bin";
  char *outFilename = "graph_out_cpy.bin";

  clock_t startClock = clock();

  while (mst.count < nodeCount - 1) {

    for (NodeId componentId = 0; componentId < nodeCount; componentId++) {
      cheapestEdges[componentId] = NOTHING;
    }

    FILE *inFile = Stream_Open(inFilename, "rb");
    FILE *outFile = Stream_Open(outFilename, "wb");

    readBuffer.count = fread(readBuffer.edges, sizeof(Edge), CHUNK_SIZE, inFile);

    while (readBuffer.count > 0) {

      auxReadBuffer.count = fread(auxReadBuffer.edges, sizeof(Edge), CHUNK_SIZE, inFile);

      EdgeChunk writeBuffer = {readBuffer.edges, 0, readBuffer.capacity};

      const Edge *readPointer = readBuffer.edges;
      const unsigned int currentChunkCount = readBuffer.count;

      for (EdgeId edgeId = 0; edgeId < currentChunkCount; edgeId++) {
        const Edge edge = readPointer[edgeId];
        const NodeId u = edge.source;
        const NodeId v = edge.destination;

        const NodeId rootU = DisjointSet_Find(&components, u);
        const NodeId rootV = DisjointSet_Find(&components, v);

        if (rootU == rootV)
          continue;

        writeBuffer.edges[writeBuffer.count++] = edge;

        Edge *cheapestEdgeOnU = &cheapestEdges[rootU];
        Edge *cheapestEdgeOnV = &cheapestEdges[rootV];

        if (edge.weight < cheapestEdgeOnU->weight) {
          *cheapestEdgeOnU = edge;
        }
        if (edge.weight < cheapestEdgeOnV->weight) {
          *cheapestEdgeOnV = edge;
        }
      }

      fwrite(writeBuffer.edges, sizeof(Edge), writeBuffer.count, outFile);

      EdgeChunk tmp = readBuffer;
      readBuffer = auxReadBuffer;
      auxReadBuffer = tmp;
    }

    Stream_Finalize(inFile);
    Stream_Finalize(outFile);

    char *temp = inFilename;
    inFilename = outFilename;
    outFilename = temp;

    for (NodeId componentId = 0; componentId < nodeCount; componentId++) {
      const Edge edge = cheapestEdges[componentId];

      if (edge.weight != UINT_MAX) {
        const NodeId u = edge.source;
        const NodeId v = edge.destination;

        if (DisjointSet_Union(&components, u, v)) {
          mst.edges[mst.count++] = (Edge){u, v, edge.weight};
        }
      }
    }
  }

  clock_t endClock = clock();
  double cpuTimeUsed = ((double)(endClock - startClock)) / CLOCKS_PER_SEC;

  long double sum = 0;
  printf("===== MST Computed =====\n");
  for (EdgeId edgeId = 0; edgeId < mst.count; edgeId++) {
    sum += mst.edges[edgeId].weight;
  }

  printf("Total MST Edge Count: %d\n", mst.count);
  printf("Expected Target Count: %d\n", nodeCount - 1);
  printf("Sum: %Lf\n", sum);
  printf("CPU Execution Time: %f seconds\n", cpuTimeUsed);

  EdgeChunk_Finalize(&readBuffer);
  EdgeChunk_Finalize(&auxReadBuffer);
  EdgeChunk_Finalize(&mst);
  DisjointSet_Finalize(&components);
  free(cheapestEdges);
  remove(inFilename);
  remove(outFilename);
}

int main(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr, "Usage: %s <graphfile> <nodeCount>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  char *filename = argv[1];

  unsigned int nodeCount;
  nodeCount = atoi(argv[2]);

  MST_ProcessMST(filename, nodeCount);

  return 0;
}
