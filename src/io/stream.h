#ifndef STREAM_H
#define STREAM_H

#include <stdio.h>
#include <sys/mman.h> 
#include <fcntl.h>    
#include <unistd.h>

#include "../core/graph.h"

Edge* Stream_Map(const char *filename, size_t *fileSize);
void Stream_Unmap(Edge *data, size_t fileSize);

#endif
