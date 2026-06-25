#ifndef MPI_TYPES_H
#define MPI_TYPES_H

#include <mpi.h>
#include "../core/graph.h"

extern MPI_Datatype MPI_EDGE_TYPE;
extern MPI_Datatype MPI_COMPONENT_EDGE_TYPE;

typedef struct {
   NodeId component;
   Edge edge;
}ComponentEdge;

void MPI_Register_Datatypes();
void MPI_Free_Datatypes();

#endif
