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

double stealPerc;
omp_lock_t stealLock;
bool verifyResults = false, throttled = false;;
int tasksStolen, tasksExecuted;

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

double execute(int tasks, int taskStealing, int *migratedTasks)
{
    double fTimeStart, fTimeEnd, elapsed = 0;
    tasksStolen = 0, tasksExecuted = 0;

    if(BCL::rank()==1)
        throttled = true;

    //setting up global data structures
    std::vector<BCL::CircularQueue<taskNoRes>> queues;
    std::vector<BCL::FastQueue<taskRes>> resultQueues;
    for (size_t rank = 0; rank < BCL::nprocs(); rank++)
    {
        queues.push_back(BCL::CircularQueue<taskNoRes>(rank, 2400));
        resultQueues.push_back(BCL::FastQueue<taskRes>(rank, 2400));
    }

    BCL::barrier();

    //create and save tasks
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

            queues[BCL::rank()].push(t, BCL::CircularQueueAL::none);

            nextId += omp_get_num_threads();
        }
    }

    BCL::barrier();
    //solve tasks and save the result at the origin rank
    fTimeStart = curtime();

    omp_init_lock(&stealLock);

#pragma omp parallel
    {
        while (true)
        {
            while (queues[BCL::rank()].size() > 0)
            {
                // cout << BCL::rank() << " " << queues[BCL::rank()].size() <<"\n";
                taskNoRes t;
                bool success = queues[BCL::rank()].pop(t);
                // cout << success << "\n";
                if (success)
                {
                    // printf("Rank %d got task %d\n", BCL::rank(), t.taskId);
                    taskRes result;
                    int ownerRank = t.taskId >> 24;
                    multiply(t.matrix, t.matrix2, result.result);
                    tasksExecuted++;
                    result.taskId = t.taskId;
                    resultQueues[ownerRank].push(result);
                }
            }

            if (queues[BCL::rank()].size() == 0 && (!taskStealing || !steal(&queues)))
            {
                break;
            }
        }
    }

    BCL::barrier();

    if (BCL::rank() == 0)
    {
        fTimeEnd = curtime();
        elapsed = fTimeEnd - fTimeStart;
        *migratedTasks += abs(max(tasks - tasksExecuted, 0) - tasksStolen);
    }

    BCL::barrier();

    return elapsed;
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
    // for (int i = 0; i < MSIZE * MSIZE; i += 1)
    // {
    //     double value = 0;
    //     int k = i % MSIZE;
    //     for (int j = (i / MSIZE) * MSIZE; j < (i / MSIZE) * MSIZE + MSIZE; j++)
    //     {
    //         value = value + matrix[j] * matrix2[k];
    //         k += MSIZE;
    //     }
    //     result[i] = value;
    // }
    
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
    if (throttled)
        for (int i = 0; i < MSIZE / 2; i++)
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
        taskNoRes t;
        long size = (*queues)[*it].size();

        if (size == 0)
            continue;

        if (stealPerc < 0)
        {
            omp_set_lock(&stealLock);
            if ((*queues)[*it].pop(t))
            {
                (*queues)[BCL::rank()].push(t);
                omp_unset_lock(&stealLock);
                tasksStolen++;
                return true;
            }
            else
            {
                omp_unset_lock(&stealLock);
                return false;
            }
        }

        //steals half the tasks
        int j = 0;
        while (j < (*queues)[*it].size() * stealPerc)
        {
            if ((*queues)[*it].pop(t))
            {
                tasksStolen++;
                (*queues)[BCL::rank()].push(t);
            }
            else
                break;

            j++;
        }

        long ownSize = (*queues)[BCL::rank()].size();
        if (ownSize > 0)
        {
            // printf("[%ld]Successfully stolen %ld/%ld tasks!\n", BCL::rank(), (*queues)[BCL::rank()].size(), size);
            return true;
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
    tmp = tmp ? tmp : "-1";
    stealPerc = atof(tmp);

    tmp = getenv("STEALING");
    tmp = tmp ? tmp : "1";
    int stealing = atoi(tmp);

    tmp = getenv("ITERATIONS");
    tmp = tmp ? tmp : "1";
    int iterations = atoi(tmp);

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

    if (BCL::rank() == 0)
        printf("Executing code with: RANKS:%ld  THREADS:%d  MSIZE:%d  STEALP:%lf  STEALING:%d  ITERATIONS:%d\n", BCL::nprocs(), omp_get_max_threads(), MSIZE, stealPerc, stealing, iterations);

    //executing program depeding on the STEALING env. variable
    double tsRuntime = 0, noTsRuntime = 0;
    int migratedTasks = 0;

    for (int i = 0; i < iterations; i++)
    {
        if (stealing > 0)
            tsRuntime += execute(tasks, 1, &migratedTasks);

        if (stealing == 0 || stealing == 2)
            noTsRuntime += execute(tasks, 0, &migratedTasks);
    }
    tsRuntime /= iterations;
    noTsRuntime /= iterations;
    migratedTasks /= iterations;

    if (BCL::rank() == 0)
    {
        if (tsRuntime > 0)
            printf("Inter Runtime: %lf\n", tsRuntime);
        if (noTsRuntime > 0)
            printf("Local Runtime: %lf\n", noTsRuntime);
        if (stealing == 2)
            printf("Speedup: %lf\n", noTsRuntime / tsRuntime);
        printf("Tasks migrated: %d\n", migratedTasks);
    }
    BCL::finalize();
}
