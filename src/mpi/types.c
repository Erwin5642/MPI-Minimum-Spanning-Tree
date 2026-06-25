#include <stddef.h>

#include "../core/graph.h"
#include "mpi.h"
#include "types.h"

MPI_Datatype MPI_EDGE_TYPE;
MPI_Datatype MPI_COMPONENT_EDGE_TYPE;

void MPI_Register_Datatypes() {
  MPI_Type_create_struct(3, (int[]){1, 1, 1},
                         (MPI_Aint[]){offsetof(Edge, source), offsetof(Edge, destination), offsetof(Edge, weight)},
                         (MPI_Datatype[]){MPI_UNSIGNED, MPI_UNSIGNED, MPI_DOUBLE}, &MPI_EDGE_TYPE);
  MPI_Type_commit(&MPI_EDGE_TYPE);

  MPI_Type_create_struct(2, (int[]){1, 1},
                         (MPI_Aint[]){offsetof(ComponentEdge, component), offsetof(ComponentEdge, edge)},
                         (MPI_Datatype[]){MPI_UNSIGNED, MPI_EDGE_TYPE}, &MPI_COMPONENT_EDGE_TYPE);
  MPI_Type_commit(&MPI_COMPONENT_EDGE_TYPE);
}

void MPI_Free_Datatypes() {
  MPI_Type_free(&MPI_COMPONENT_EDGE_TYPE);
  MPI_Type_free(&MPI_EDGE_TYPE);
}
