#include <bcl/bcl.hpp>
#include <bcl/containers/FastQueue.hpp>
#include <bcl/containers/CircularQueue.hpp>
#include <string>
#include <cstdlib>
#include <time.h>
#include <omp.h>

#include <random>

double fTimeStart, fTimeEnd;
bool taskStealing = true;

void multiply(double *matrix, double *matrix2, double *result, int matrixSize);
void initialize_matrix_rnd(double *mat, int matrixSize);
bool steal(std::vector<BCL::CircularQueue<task>> *queues, bool intraNode);

static inline double curtime(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec * 1e-9;
}

int main(int argc, char **argv)
{
    BCL::init();

    int matrixSize = atoi(argv[1]);
    int tasks = 0;

    //get the number of tasks to enqueue
    int rankOffset = 0;
    for (int i = 2; i < argc; i += 2)
    {
        if (atoi(argv[i + 1]) + rankOffset > BCL::rank() && rankOffset <= BCL::rank())
        {
            tasks = atoi(argv[i]);
        }
        rankOffset += atoi(argv[i + 1]);
    }

    std::vector<BCL::CircularQueue<task>> queues;
    for (size_t rank = 0; rank < BCL::nprocs(); rank++)
    {
        for (size_t thread = 0; thread < omp_get_max_threads(); thread++)
        {
            queues.push_back(BCL::CircularQueue<task>(rank, 60000 / omp_get_num_threads()));
        }
    }

    //create tasks
    for (int i = 0; i < tasks; i++)
    {
        struct task t;
        t.matrix = (double *)malloc(sizeof(double) * matrixSize * matrixSize);
        t.matrix2 = (double *)malloc(sizeof(double) * matrixSize * matrixSize);
        t.result = (double *)malloc(sizeof(double) * matrixSize * matrixSize);
        t.matrixSize = matrixSize;
        initialize_matrix_rnd(t.matrix, matrixSize);
        initialize_matrix_rnd(t.matrix2, matrixSize);
        queues[BCL::rank() * omp_get_num_threads() + omp_get_thread_num()].push(t, BCL::CircularQueueAL::push);
    }
    BCL::barrier();
    // for (int i = 0; i < BCL::nprocs(); i++)
    //    printf("[%ld]Rank %d:%ld\n", BCL::rank(), i, queues[i].size());

    //solve tasks
    if (BCL::rank() == 0)
        fTimeStart = curtime();

#pragma omp parallel
    {
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

            task t;
            bool success = queues[BCL::rank() * omp_get_num_threads() + omp_get_thread_num()].pop(t);
            if (success)
            {
                multiply(t.matrix, t.matrix2, t.result, matrixSize);
                free(t.matrix);
                free(t.matrix2);
                free(t.result);
            }
        }
    }
    BCL::barrier();

    if (BCL::rank() == 0)
    {
        fTimeEnd = curtime();
        double elapsed = fTimeEnd - fTimeStart;
        printf("%lf\n", elapsed);
    }

    BCL::finalize();
}

void initialize_matrix_rnd(double *mat, int matrixSize)
{
    double lower_bound = 0;
    double upper_bound = 10;
    std::uniform_real_distribution<double> unif(lower_bound, upper_bound);
    std::default_random_engine re;

    for (int i = 0; i < matrixSize * matrixSize; i++)
    {
        mat[i] = unif(re);
    }
}

void multiply(double *matrix, double *matrix2, double *result, int matrixSize)
{
    for (int i = 0; i < matrixSize * matrixSize; i += 1)
    {
        double value = 0;
        int k = i % matrixSize;
        for (int j = (i / matrixSize) * matrixSize; j < (i / matrixSize) * matrixSize + matrixSize; j++)
        {
            value = value + matrix[j] * matrix2[k];
            k += matrixSize;
        }
        result[i] = value;
    }
}

bool steal(std::vector<BCL::CircularQueue<task>> *queues, bool intraNode)
{
    std::srand(unsigned(std::time(0)));
    std::vector<int> ranks;

    // get queue positions of a threads inside of own rank
    if (intraNode)
    {
        for (int i = BCL::rank()*omp_get_num_threads(); i < (BCL::rank()+1)*omp_get_num_threads(); i++)
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
            if (i < BCL::rank() * omp_get_num_threads() || i>(BCL::rank()+1)*omp_get_num_threads())
                ranks.push_back(i);
        }
    }

    std::random_shuffle(ranks.begin(), ranks.end());

    int i = (BCL::rank() + 1) % BCL::nprocs();
    //iterates through ranks and tests if they have tasks left
    for (std::vector<int>::iterator it = ranks.begin(); it != ranks.end(); ++it)
    {
        // printf("[%ld]Current: %d\n", BCL::rank(), *it);
        long size = (*queues)[*it].size();
        if (size > 1)
        {
            //printf("[%ld]found %d!\n", BCL::rank(),i);
            task t;
            //steals half the tasks
            for (int j = 0; j < (*queues)[*it].size() / 2; j++)
            {
                (*queues)[*it].pop(t);
                //printf("[%ld]stealing task %d\n", BCL::rank(), j);
                (*queues)[BCL::rank() * omp_get_num_threads() + omp_get_thread_num()].push(t);
            }
            long ownSize = (*queues)[BCL::rank() * omp_get_num_threads() + omp_get_thread_num()].size();
            if (ownSize > 0)
            {
                printf("[%ld]Successfully stolen %ld/%ld tasks!\n", BCL::rank(), (*queues)[BCL::rank() * omp_get_num_threads() + omp_get_thread_num()].size(), size);
                return true;
            }
        }
    }

    if(intraNode)
        return steal(queues, false);
    return false;
}
