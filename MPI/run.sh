
# Parameters:
#   $1: Number of processes
#   $2: Upper range for euler totient (optional)
#   $3: Lower range for euler totient (optional)
mpirun -n $1 ./mpi_euler $2 $3
