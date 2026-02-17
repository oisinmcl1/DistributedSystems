#include <iostream>
#include <mpi.h>

int main(int argc, char** argv) {
    // Initialize MPI (MUST be first MPI call!)
    MPI_Init(&argc, &argv);

    // Get my rank ID and total number of processes
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // "Who am I?"
    MPI_Comm_size(MPI_COMM_WORLD, &size); // "How many processes total?"

    // Each process prints its identity
    std::cout << "Hello from rank " << rank << " of " << size << "\n";
    // Cleanup MPI (MUST be last MPI call!)
    MPI_Finalize();
    return 0;
}