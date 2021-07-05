#include <bcl/bcl.hpp>
#include <bcl/containers/CircularQueue.hpp>
#include <bcl/containers/FastQueue.hpp>
#include <string>
#include <cstdlib>
#include <time.h>
#include <omp.h>
#include <algorithm>
#include <bitset>
#include <random>

bool verifyResults = false,
     ShowRuntimeDist = true;

double stealPerc, idleTime, multiplyTime, communicationTime, lockTime;
omp_lock_t stealLock;

int tasksStolen = 0;

void multiply(double *matrix, double *matrix2, double *result);
void initialize_matrix_rnd(double *mat);
bool steal(std::vector<BCL::CircularQueue<taskNoRes>> *queues);

using namespace std;

static inline double curtime(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec * 1e-9;
}

double execute(int tasks, int taskStealing)
{
    double executionTimeStart, executionTime = 0, cumulativeTime = 0;
    idleTime = 0;
    multiplyTime = 0, communicationTime = 0, lockTime=0;
    tasksStolen = 0;

    if (BCL::rank() == 0)
        printf("Executing code with: RANKS:%ld  THREADS:%d  MSIZE:%d  STEALP:%lf  STEALING:%d\n", BCL::nprocs(), omp_get_max_threads(), MSIZE, stealPerc, taskStealing);

    //setting up global data structures
    std::vector<BCL::CircularQueue<taskNoRes>> queues;
    std::vector<BCL::CircularQueue<taskRes>> resultQueues;
    for (size_t rank = 0; rank < BCL::nprocs(); rank++)
    {
        queues.push_back(BCL::CircularQueue<taskNoRes>(rank, 10000));
        resultQueues.push_back(BCL::CircularQueue<taskRes>(rank, 10000));
    }

    BCL::barrier();

    //create and save tasks
    omp_init_lock(&stealLock);
#pragma omp parallel
    {
        int nextId = omp_get_thread_num();
#pragma omp for
        for (int i = 0; i < tasks; i++)
        {
            taskNoRes t;
            t.matrixSize = MSIZE;

            t.taskId = (int)(BCL::rank());
            t.taskId <<= 24;
            t.taskId += nextId;

            initialize_matrix_rnd(t.matrix);
            initialize_matrix_rnd(t.matrix2);

            queues[BCL::rank()].push(t, BCL::CircularQueueAL::push);

            nextId += omp_get_num_threads();
        }
    }

    BCL::barrier();
    //solve tasks and save the result at the origin rank

    executionTimeStart = curtime();

#pragma omp parallel
    {
        double threadTimeStart = curtime();
        while (true)
        {
            vector<double> threadTime;
            for (int i = 0; i < omp_get_max_threads(); i++)
            {
                threadTime.push_back(0);
            }

            bool emptyQueue = false;
            for (int i = 0; i < queues[BCL::rank()].size(); i++)
            {
                if (emptyQueue)
                    continue;
                // cout << BCL::rank() << " " << queues[BCL::rank()].size() <<"\n";
                taskNoRes t;
                bool success = queues[BCL::rank()].pop(t);
                // cout << success << "\n";
                if (success)
                {
                    int ownerRank = t.taskId >> 24;
                    taskRes result;

                    // printf("[%ld, %ld]Processing task %d %d\n", BCL::rank(), omp_get_thread_num(), ownerRank, taskIndex);
                    double multiplyTimeStart = curtime();
                    multiply(t.matrix, t.matrix2, result.result);
                    multiplyTime += curtime() - multiplyTimeStart;

                    result.taskId = t.taskId;

                    double communicationTimeStart = curtime();
                    resultQueues[ownerRank].push(result);
                    communicationTime += curtime() - communicationTimeStart;
                }
                else
                    emptyQueue = true;
            }

            if (queues[BCL::rank()].size() == 0 && (!taskStealing || !steal(&queues)))
            {
                threadTime[omp_get_thread_num()] += curtime() - threadTimeStart;
                for (int i = 0; i < omp_get_max_threads(); i++)
                {
                    cumulativeTime += threadTime[i];
                }
                break;
            }
        }
    }

    executionTime = curtime() - executionTimeStart;
    idleTime = cumulativeTime - multiplyTime;
    if (ShowRuntimeDist)
    {
        printf("[%ld]Cumulative benchmarks of all threads:\n   ", BCL::rank());
        printf("-----------------------------------\n   ");
        printf("Processor time:        %lf\n   ", cumulativeTime);
        printf("-----------------------------------\n   ");
        printf("Multiplication time:    %lf = %lf%%\n   ", multiplyTime, multiplyTime / cumulativeTime * 100);
        printf("Idle time:              %lf = %lf%%\n   ", idleTime, idleTime / cumulativeTime * 100);
        printf("-----------------------------------\n   ");
        printf("Communication time:     %lf = %lf%%\n   ", communicationTime, communicationTime / cumulativeTime * 100);
        printf("Lock time:              %lf = %lf%%\n   ", lockTime, lockTime / cumulativeTime * 100);
        printf("Other:                  %lf = %lf%%\n   ", idleTime - communicationTime - lockTime, (idleTime - communicationTime - lockTime) / cumulativeTime * 100);
        printf("-----------------------------------\n   ");
        printf("Tasks stolen:           %d\n", tasksStolen);
    }

    BCL::barrier();

    if (BCL::rank() == 0)
    {
        printf("Total execution time: %lf\n", executionTime);
        fflush(stdout);
    }

    BCL::barrier();

    return executionTime;
}

void initialize_matrix_rnd(double *mat)
{
    double lower_bound = 0;
    double upper_bound = 10;
    std::uniform_real_distribution<double> unif(lower_bound, upper_bound);
    std::default_random_engine re;

    for (int i = 0; i < MSIZE * MSIZE; i++)
    {
        mat[i] = unif(re);
    }
}

void multiply(double *matrix, double *matrix2, double *result)
{
    for (int i = 0; i < MSIZE; i++)
    {
        for (int j = 0; j < MSIZE; j++)
        {
            result[i * MSIZE + j] = 0;
            for (int k = 0; k < MSIZE; k++)
            {
                result[i * MSIZE + j] += matrix[i * MSIZE + k] * matrix2[k * MSIZE + j];
            }
        }
    }
}

bool steal(std::vector<BCL::CircularQueue<taskNoRes>> *queues)
{
    std::srand(unsigned(std::time(0)));
    std::vector<int> ranks;

    for (int i = 0; i < BCL::nprocs(); i++)
    {
        if (i != BCL::rank())
            ranks.push_back(i);
    }

    std::random_shuffle(ranks.begin(), ranks.end());

    //iterates through ranks and tests if they have tasks left
    for (std::vector<int>::iterator it = ranks.begin(); it != ranks.end(); ++it)
    {

        double communicationTimeStart = curtime();
        long size = (*queues)[*it].size();
        communicationTime += curtime() - communicationTimeStart;

        if (size > 0)
        {
            taskNoRes t;

            //Steals one task
            if (stealPerc < 0)
            {
                double lockTimeStart = curtime();
                omp_set_lock(&stealLock);
                lockTime += curtime() - lockTimeStart;
                communicationTimeStart = curtime();
                if ((*queues)[*it].pop(t))
                {
                    (*queues)[BCL::rank()].push(t);
                    communicationTime += curtime() - communicationTimeStart;
                    tasksStolen++;
                    lockTimeStart = curtime();
                    omp_unset_lock(&stealLock);
                    lockTime += curtime() - lockTimeStart;
                    return true;
                }
                else
                {
                    omp_unset_lock(&stealLock);
                    communicationTime += curtime() - communicationTimeStart;
                    return false;
                }
            }
            //steals half the tasks
            int j = 0;
            while (j < (*queues)[*it].size() * stealPerc)
            {
                communicationTimeStart = curtime();
                if ((*queues)[*it].pop(t))
                {
                    (*queues)[BCL::rank()].push(t);
                    communicationTime += curtime() - communicationTimeStart;
                }
                else
                {
                    communicationTime += curtime() - communicationTimeStart;
                    break;
                }

                j++;
            }

            communicationTimeStart = curtime();
            long ownSize = (*queues)[BCL::rank()].size();
            communicationTime += curtime() - communicationTimeStart;
            if (ownSize > 0)
            {
                tasksStolen += ownSize;
                // printf("[%ld]Successfully stolen %ld/%ld tasks!\n", BCL::rank(), (*queues)[BCL::rank()].size(), size);
                return true;
            }
        }
    }

    return false;
}

int main(int argc, char **argv)
{
    BCL::init(30 * 256, true);

    //initializing variables
    int tasks = 0;

    //obtaining env. variables
    const char *tmp = getenv("STEALP");
    tmp = tmp ? tmp : "0.5";
    stealPerc = atof(tmp);

    tmp = getenv("STEALING");
    tmp = tmp ? tmp : "1";
    int stealing = atoi(tmp);

    //get the number of tasks to enqueue
    int rankOffset = 0;
    for (int i = 1; i < argc; i += 2)
    {
        if (atoi(argv[i + 1]) + rankOffset > BCL::rank() && rankOffset <= BCL::rank())
        {
            tasks = atoi(argv[i]);
        }
        rankOffset += atoi(argv[i + 1]);
    }

    //executing program depeding on the STEALING env. variable
    double tsRuntime, noTsRuntime;
    if (stealing > 0)
        tsRuntime = execute(tasks, 1);

    if (stealing == 0 || stealing == 2)
        noTsRuntime = execute(tasks, 0);

    if (BCL::rank() == 0 && stealing == 2)
        printf("Speedup: %lf\n", noTsRuntime / tsRuntime);

    BCL::finalize();
}
