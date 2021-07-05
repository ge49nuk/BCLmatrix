export OMP_PROC_BIND=true
export OMP_PLACES=cores
unset KMP_AFFINITY
export I_MPI_PIN_DOMAIN=auto
export I_MPI_PIN=1
module load intel-mpi-2019.10.317-gcc-10.2.1-n23qoov
