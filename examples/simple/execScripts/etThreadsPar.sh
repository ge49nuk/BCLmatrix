cd ..
export STEALING=2
export ITERATIONS=5
export STEALP=-1
export OMP_NUM_THREADS=1
mpirun -n 2 -ppn 1 --host 10.12.1.1,10.12.1.2 ./comparableTSpar 2400 1  > execScripts/Outputs/etThreadsPar.txt
export OMP_NUM_THREADS=2
mpirun -n 2 -ppn 1 --host 10.12.1.1,10.12.1.2 ./comparableTSpar 2400 1  >> execScripts/Outputs/etThreadsPar.txt
export OMP_NUM_THREADS=6
mpirun -n 2 -ppn 1 --host 10.12.1.1,10.12.1.2 ./comparableTSpar 2400 1 >> execScripts/Outputs/etThreadsPar.txt
export OMP_NUM_THREADS=10
mpirun -n 2 -ppn 1 --host 10.12.1.1,10.12.1.2 ./comparableTSpar 2400 1 >> execScripts/Outputs/etThreadsPar.txt
export OMP_NUM_THREADS=14
mpirun -n 2 -ppn 1 --host 10.12.1.1,10.12.1.2 ./comparableTSpar 2400 1 >> execScripts/Outputs/etThreadsPar.txt
export OMP_NUM_THREADS=18
mpirun -n 2 -ppn 1 --host 10.12.1.1,10.12.1.2 ./comparableTSpar 2400 1 >> execScripts/Outputs/etThreadsPar.txt
export OMP_NUM_THREADS=22
mpirun -n 2 -ppn 1 --host 10.12.1.1,10.12.1.2 ./comparableTSpar 2400 1 >> execScripts/Outputs/etThreadsPar.txt
export OMP_NUM_THREADS=26
mpirun -n 2 -ppn 1 --host 10.12.1.1,10.12.1.2 ./comparableTSpar 2400 1 >> execScripts/Outputs/etThreadsPar.txt
export OMP_NUM_THREADS=30
mpirun -n 2 -ppn 1 --host 10.12.1.1,10.12.1.2 ./comparableTSpar 2400 1 >> execScripts/Outputs/etThreadsPar.txt
