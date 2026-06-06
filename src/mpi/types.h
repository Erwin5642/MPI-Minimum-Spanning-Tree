#ifndef MPI_TYPES_H
#define MPI_TYPES_H

#include <mpi.h>

extern MPI_Datatype MPI_EDGE_TYPE;

void MPI_Register_Datatypes();
void MPI_Free_Datatypes();

#endif
