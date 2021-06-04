#!/bin/bash
#SBATCH -J MpiThreads
#SBATCH -o /dss/dssfs02/lwp-dss-0001/pr63so/pr63so-dss-0000/ge49nuk2/bcl/benchmarks/%x.%j.%N.out
#SBATCH -D /dss/dssfs02/lwp-dss-0001/pr63so/pr63so-dss-0000/ge49nuk2/bcl/examples/simple/execScripts
#SBATCH --clusters=inter
#SBATCH --partition=mpp3_inter
#SBATCH --get-user-env
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=1
#SBATCH --mail-type=end
#SBATCH --mail-user=nikolan00@gmail.com
#SBATCH --export=NONE
#SBATCH --time=02:00:00
module load slurm_setup
module load intel/19.0.5
module load intel-mpi/2019.8.254
./etThreads
