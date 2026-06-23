#ifndef STREAM_H
#define STREAM_H

#include <stddef.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include "../core/graph.h"

Edge *Stream_Map(const char *graphFilePath, size_t *fileSizeBytes);
void Stream_Unmap(Edge *edgeList, size_t fileSizeBytes);

#endif
