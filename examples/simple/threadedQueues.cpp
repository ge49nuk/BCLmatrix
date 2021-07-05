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
bool verifyResults = false;
omp_lock_t stealLock;
int tasksStolen, tasksExecuted;

void multiply(double *matrix, double *matrix2, double *result);
void initialize_matrix_rnd(double *mat);
bool steal(std::vector<BCL::CircularQueue<taskNoRes>> *queues, bool intraNode);

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

    //setting up global data structures
    std::vector<BCL::CircularQueue<taskNoRes>> queues;
    std::vector<BCL::FastQueue<taskRes>> resultQueues;
    for (size_t rank = 0; rank < BCL::nprocs(); rank++)
    {
        for (size_t thread = 0; thread < omp_get_max_threads(); thread++)
        {
            queues.push_back(BCL::CircularQueue<taskNoRes>(rank, 3000 / omp_get_max_threads()));
            resultQueues.push_back(BCL::FastQueue<taskRes>(rank, 3000 / omp_get_max_threads()));
        }
    }

    BCL::barrier();
    omp_init_lock(&stealLock);

    //create and save tasks
#pragma omp parallel
    {
        int myThreadTasks;
        int nextId = 0;
        myThreadTasks = tasks / omp_get_num_threads();
        if (omp_get_thread_num() < tasks % omp_get_num_threads())
            myThreadTasks = myThreadTasks + 1;

        //create tasks
        for (int i = 0; i < myThreadTasks; i++)
        {
            taskNoRes t;
            t.matrixSize = MSIZE;
            t.taskId = (int)(BCL::rank() * omp_get_num_threads() + omp_get_thread_num());

            t.taskId <<= 24;
            t.taskId += nextId;
            nextId++;
            initialize_matrix_rnd(t.matrix);
            initialize_matrix_rnd(t.matrix2);
            queues[BCL::rank() * omp_get_num_threads() + omp_get_thread_num()].push(t, BCL::CircularQueueAL::push);
        }

#pragma omp barrier

        BCL::barrier();
        //solve tasks and save the result at the origin rank

        fTimeStart = curtime();

        while (true)
        {
            //steal tasks if activated
            if (queues[BCL::rank() * omp_get_num_threads() + omp_get_thread_num()].empty())
            {
                if (!taskStealing)
                    break;
                if (!steal(&queues, true))
                {
                    break;
                }
            }

            taskNoRes t;
            bool success = queues[BCL::rank() * omp_get_num_threads() + omp_get_thread_num()].pop(t);
            // printf("[%ld] [%ld] Thread\n", BCL::rank(), omp_get_thread_num());
            if (success)
            {
                taskRes result;
                int ownerRank = t.taskId >> 24;
                multiply(t.matrix, t.matrix2, result.result);
                result.taskId = t.taskId;
                resultQueues[ownerRank].push(result);
            }
        }
        // int flag;

        // printf("%d\n",MPI_Iprobe(MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE));
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

bool steal(std::vector<BCL::CircularQueue<taskNoRes>> *queues, bool intraNode)
{
    std::srand(unsigned(std::time(0)));
    std::vector<int> ranks;

    // get queue positions of a threads inside of own rank
    if (intraNode)
    {
        for (int i = BCL::rank() * omp_get_num_threads(); i < (BCL::rank() + 1) * omp_get_num_threads(); i++)
        {
            if (i != BCL::rank() * omp_get_num_threads() + omp_get_thread_num())
                ranks.push_back(i);
        }
    }
    //  "" outside of own rank
    else
    {
        for (int i = 0; i < BCL::nprocs() * omp_get_num_threads(); i++)
        {
            if (i < BCL::rank() * omp_get_num_threads() || i > (BCL::rank() + 1) * omp_get_num_threads())
                ranks.push_back(i);
        }
    }

    std::random_shuffle(ranks.begin(), ranks.end());

    int i = (BCL::rank() + 1) % BCL::nprocs();
    //iterates through ranks and tests if they have tasks left
    for (std::vector<int>::iterator it = ranks.begin(); it != ranks.end(); ++it)
    {
        // printf("[%ld]Current: %d\n", BCL::rank(), *it);
        //printf("[%ld][%ld]Stealing from %d\n", BCL::rank(), omp_get_thread_num(), *it);
        long size = (*queues)[*it].size();
        if (size == 0)
            continue;

        //printf("[%ld]found %d!\n", BCL::rank(),i);
        taskNoRes t;


        //steals tasks
        int j = 0;
        do{
            if ((*queues)[*it].pop(t))
                (*queues)[BCL::rank() * omp_get_num_threads() + omp_get_thread_num()].push(t);

            j++;
        }while (j < (*queues)[*it].size() * stealPerc);

        long ownSize = (*queues)[BCL::rank() * omp_get_num_threads() + omp_get_thread_num()].size();
        if (ownSize > 0)
        {
            // printf("[%ld]Successfully stolen %ld/%ld tasks!\n", BCL::rank() * omp_get_num_threads() + omp_get_thread_num(), (*queues)[BCL::rank() * omp_get_num_threads() + omp_get_thread_num()].size(), size);
            return true;
        }
    }

    if (intraNode)
        return steal(queues, false);
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
        // printf("Tasks migrated: %d\n", migratedTasks);
    }
    BCL::finalize();
}
