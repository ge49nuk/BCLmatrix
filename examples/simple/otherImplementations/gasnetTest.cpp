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
#include <queue>

using namespace std;

static inline double curtime(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec * 1e-9;
}

int main(int argc, char **argv)
{
    BCL::init(30 * 256, true);
    double tasks = atoi(argv[1]);

    std::vector<BCL::CircularQueue<int>> queues;
    for (size_t rank = 0; rank < BCL::nprocs(); rank++)
    {
        queues.push_back(BCL::CircularQueue<int>(rank, 999999));
    }
    if (BCL::rank() == 0)
    {
        for (int i = 0; i < tasks; i += 1)
        {
            queues[0].push(i);
        }
    }
    BCL::barrier();
    double start = curtime();
        while (0 < queues[0].size())
        {
            int k;
            queues[0].pop(k);
            double a = sqrt(sqrt(k));
            printf("[%ld]Calced %lf\n", BCL::rank(), a);
        }
    BCL::barrier();

    if (BCL::rank() == 0)
        printf("%lf\n", curtime() - start);

    BCL::finalize();
}
