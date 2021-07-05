cd ..
export STEALING=2
export ITERATIONS=5
export STEALP=-1
export OMP_NUM_THREADS=1
mpirun -n 2 -ppn 1 --host 10.12.1.1,10.12.1.2 ./comparableTSpar 2400 1  > execScripts/Outputs/etThreads.txt
mpirun -n 2 -ppn 1 --host 10.12.1.1,10.12.1.2 ./comparableTSpar 2400 1  >> execScripts/Outputs/etThreads.txt
mpirun -n 2 -ppn 1 --host 10.12.1.1,10.12.1.2 ./comparableTSpar 2400 1  >> execScripts/Outputs/etThreads.txt
export OMP_NUM_THREADS=2
mpirun -n 2 -ppn 1 --host 10.12.1.1,10.12.1.2 ./comparableTSpar 2400 1  >> execScripts/Outputs/etThreads.txt
mpirun -n 2 -ppn 1 --host 10.12.1.1,10.12.1.2 ./comparableTSpar 2400 1  >> execScripts/Outputs/etThreads.txt
mpirun -n 2 -ppn 1 --host 10.12.1.1,10.12.1.2 ./comparableTSpar 2400 1  >> execScripts/Outputs/etThreads.txt
export OMP_NUM_THREADS=4
mpirun -n 2 -ppn 1 --host 10.12.1.1,10.12.1.2 ./comparableTSpar 2400 1 >> execScripts/Outputs/etThreads.txt
mpirun -n 2 -ppn 1 --host 10.12.1.1,10.12.1.2 ./comparableTSpar 2400 1  >> execScripts/Outputs/etThreads.txt
mpirun -n 2 -ppn 1 --host 10.12.1.1,10.12.1.2 ./comparableTSpar 2400 1  >> execScripts/Outputs/etThreads.txt
export OMP_NUM_THREADS=8
mpirun -n 2 -ppn 1 --host 10.12.1.1,10.12.1.2 ./comparableTSpar 2400 1 >> execScripts/Outputs/etThreads.txt
mpirun -n 2 -ppn 1 --host 10.12.1.1,10.12.1.2 ./comparableTSpar 2400 1  >> execScripts/Outputs/etThreads.txt
mpirun -n 2 -ppn 1 --host 10.12.1.1,10.12.1.2 ./comparableTSpar 2400 1  >> execScripts/Outputs/etThreads.txt
export OMP_NUM_THREADS=16
mpirun -n 2 -ppn 1 --host 10.12.1.1,10.12.1.2 ./comparableTSpar 2400 1 >> execScripts/Outputs/etThreads.txt
mpirun -n 2 -ppn 1 --host 10.12.1.1,10.12.1.2 ./comparableTSpar 2400 1  >> execScripts/Outputs/etThreads.txt
mpirun -n 2 -ppn 1 --host 10.12.1.1,10.12.1.2 ./comparableTSpar 2400 1  >> execScripts/Outputs/etThreads.txt
export OMP_NUM_THREADS=32
mpirun -n 2 -ppn 1 --host 10.12.1.1,10.12.1.2 ./comparableTSpar 2400 1 >> execScripts/Outputs/etThreads.txt
mpirun -n 2 -ppn 1 --host 10.12.1.1,10.12.1.2 ./comparableTSpar 2400 1  >> execScripts/Outputs/etThreads.txt
mpirun -n 2 -ppn 1 --host 10.12.1.1,10.12.1.2 ./comparableTSpar 2400 1  >> execScripts/Outputs/etThreads.txt
