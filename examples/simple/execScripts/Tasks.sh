cd ..
export STEALING=2
export OMP_NUM_THREADS=16
export STEALP=0.5
echo "Execution 1"
mpirun -n 2 -ppn 1 --host rome1,rome2 ./taskStealing 400 1  > execScripts/Outputs/Tasks.txt
echo "Execution 2"
mpirun -n 2 -ppn 1 --host rome1,rome2 ./taskStealing 800 1  >> execScripts/Outputs/Tasks.txt
echo "Execution 3"
mpirun -n 2 -ppn 1 --host rome1,rome2 ./taskStealing 1200 1  >> execScripts/Outputs/Tasks.txt
echo "Execution 4"
mpirun -n 2 -ppn 1 --host rome1,rome2 ./taskStealing 1600 1  >> execScripts/Outputs/Tasks.txt
echo "Execution 5"
mpirun -n 2 -ppn 1 --host rome1,rome2 ./taskStealing 2000 1  >> execScripts/Outputs/Tasks.txt
echo "Execution 6"
mpirun -n 2 -ppn 1 --host rome1,rome2 ./taskStealing 2400 1  >> execScripts/Outputs/Tasks.txt
echo "Execution 7"
mpirun -n 2 -ppn 1 --host rome1,rome2 ./taskStealing 2800 1  >> execScripts/Outputs/Tasks.txt
echo "Execution 8"
mpirun -n 2 -ppn 1 --host rome1,rome2 ./taskStealing 3200 1  >> execScripts/Outputs/Tasks.txt

