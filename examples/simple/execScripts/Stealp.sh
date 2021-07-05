cd ..
export STEALING=2
export ITERATIONS=5
export STEALP=0.1
export OMP_NUM_THREADS=22
mpirun -n 2 -ppn 1 --host 10.12.1.1,10.12.1.2 ./comparableTS 2400 1  > execScripts/Outputs/stealp.txt
export STEALP=0.2
mpirun -n 2 -ppn 1 --host 10.12.1.1,10.12.1.2 ./comparableTS 2400 1  >> execScripts/Outputs/stealp.txt
export STEALP=0.3
mpirun -n 2 -ppn 1 --host 10.12.1.1,10.12.1.2 ./comparableTS 2400 1  >> execScripts/Outputs/stealp.txt
export STEALP=0.4
mpirun -n 2 -ppn 1 --host 10.12.1.1,10.12.1.2 ./comparableTS 2400 1  >> execScripts/Outputs/stealp.txt
export STEALP=0.5
mpirun -n 2 -ppn 1 --host 10.12.1.1,10.12.1.2 ./comparableTS 2400 1  >> execScripts/Outputs/stealp.txt