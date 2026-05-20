# Performance Analysis of Borůvka's MST Algorithm in Distributed Memory Architectures using C and OpenMPI

This repository contains the source code, automation scripts, and analytical benchmarking developed for a comparative study of sequential and parallel implementations of Borůvka's algorithm for finding the Minimum Spanning Tree (MST) of a graph. The core objective of this project is to analyze high-performance computing (HPC) efficiency of using **OpenMPI** in C.

---

## Project Overview

Borůvka's algorithm (1926) possesses a unique structural property: it allows multiple connected components to be processed simultaneously and independently during each graph contraction step. Unlike Prim's or Kruskal's algorithms, which choose edges via centralized or strictly sequential data structures, Borůvka’s decentralized nature makes it highly suitable for massive parallelization.

This project evaluates the challenges and performance gains of porting this algorithm to distributed memory systems:
1. **Sequential Approach:** A canonical C implementation with an \(\mathcal{O}(E \log V)\) time complexity, serving as our single-threaded performance baseline.
2. **Parallel (Distributed) Approach:** A message-passing implementation using **OpenMPI** in C. Edge lists are partitioned across multiple processing ranks, necessitating point-to-point and collective communication to achieve global MST reduction.

---

## Technology Stack and Environment

* **Language:** C (ISO C99 standard or higher)
* **Parallel API:** OpenMPI (Message Passing Interface)
* **Compiler:** `mpicc` 

---

## 📂 Repository Structure

```text
├── bin/                      # Compiled final executable binaries
├── build/                    # Intermediary object files (.o)
├── src/                      # Project source code
│   ├── sequencial/           # Canonical single-threaded C code
│   └── paralelo/             # Parallel C + OpenMPI code
├── scripts/                  # Bash automation for job submission
├── data/                     # Text files containing test graphs (V, E, and weights)
├── docs/                     # Academic report and generated efficiency charts
├── Makefile                  # Build automation file
└── README.md                 # Main documentation
```

---

## Compilation and Execution

The project uses a `Makefile` to manage the build, isolating object files into `/build` and binaries into `/bin`.

### Prerequisites
Ensure the `make` utility and an OpenMPI distribution are correctly installed on your local machine or cluster:
```bash
# Ubuntu/Debian installation example
sudo apt-get update
sudo apt-get install build-essential openmpi-bin libopenmpi-dev
```

### Build Instructions
The `Makefile` automatically sets up the `/build` and `/bin` directories. Use the targets below to compile the project:

```bash
# Compile both sequential and parallel versions
make all

# Compile only the sequential baseline
make sequential

# Compile only the parallel OpenMPI version
make parallel

# Remove all generated object files and binaries
make clean
```

### Run Instructions
Executables must be invoked from the root of the repository, passing the graph input file path as an argument:

```bash
# Execute the sequential baseline
./bin/sequential data/graph_example.txt

# Execute the parallel version locally with 4 MPI processes
mpirun -np 4 ./bin/parallel data/graph_example.txt

# Execute within a cluster environment using a Hostfile (16 processes)
mpirun --hostfile hostfile -np 16 ./bin/parallel data/graph_example.txt
```

---

## Authors and Acknowledgments

* **Author:** [João Vitor Antunes da Silva]
* **Instructor:** [Rubens Barbosa Filho]
* **Institution:** [Universidade Estadual de Mato Grosso do Sul (UEMS)]
*
