#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "stream.h"

Edge* Stream_Map(const char *filename, size_t *fileSize) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Error: could not open file '%s': %s\n", filename, strerror(errno));
        exit(EXIT_FAILURE);
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        fprintf(stderr, "Error: could not stat file '%s': %s\n", filename, strerror(errno));
        close(fd);
        exit(EXIT_FAILURE);
    }

    *fileSize = st.st_size;

    if (*fileSize == 0) {
        fprintf(stderr, "Error: file '%s' is empty\n", filename);
        close(fd);
        exit(EXIT_FAILURE);
    }

    Edge *data = mmap(NULL, *fileSize, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    close(fd);

    if (data == MAP_FAILED) {
        fprintf(stderr, "Error: mmap failed for '%s': %s\n", filename, strerror(errno));
        exit(EXIT_FAILURE);
    }

    return data;
}

void Stream_Unmap(Edge *data, size_t fileSize) {
    munmap(data, fileSize);
}
