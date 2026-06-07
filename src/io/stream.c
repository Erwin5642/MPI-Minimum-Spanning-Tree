#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "stream.h"

Edge *Stream_Map(const char *graphFilePath, size_t *fileSizeBytes) {
  int fileDescriptor = open(graphFilePath, O_RDONLY);
  if (fileDescriptor == -1) {
    fprintf(stderr, "Erro: Stream: não foi possível abrir o arquivo '%s': %s\n", graphFilePath, strerror(errno));
    return NULL;
  }

  struct stat fileStat;
  if (fstat(fileDescriptor, &fileStat) == -1) {
    fprintf(stderr, "Erro: Stream: não foi possível obter informações do arquivo '%s': %s\n", graphFilePath,
            strerror(errno));
    close(fileDescriptor);
    return NULL;
  }

  *fileSizeBytes = fileStat.st_size;

  if (*fileSizeBytes == 0) {
    fprintf(stderr, "Erro: Stream: o arquivo '%s' está vazio\n", graphFilePath);
    close(fileDescriptor);
    errno = EINVAL;
    return NULL;
  }

  Edge *edgeList = mmap(NULL, *fileSizeBytes, PROT_READ | PROT_WRITE, MAP_PRIVATE, fileDescriptor, 0);
  close(fileDescriptor);

  if (edgeList == MAP_FAILED) {
    fprintf(stderr, "Erro: Stream: falha no mmap do arquivo '%s': %s\n", graphFilePath, strerror(errno));
    return NULL;
  }

  return edgeList;
}

void Stream_Unmap(Edge *edgeList, size_t fileSizeBytes) {
  munmap(edgeList, fileSizeBytes);
}
