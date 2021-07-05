#include <bcl/bcl.hpp>
#include <bcl/containers/CircularQueue.hpp>
#include <cstdlib>
#include <time.h>
#include <omp.h>
#include <algorithm>

using namespace std;
omp_lock_t stealLock;

int main(int argc, char **argv)
{
    BCL::init(30 * 256, true);
    omp_init_lock(&stealLock);
    double tasks = atoi(argv[1]);
    int extraTasks = 100;

    std::vector<BCL::CircularQueue<int>> queues;
    for (size_t rank = 0; rank < BCL::nprocs(); rank++)
    {
        queues.push_back(BCL::CircularQueue<int>(rank, 3000));
    }
    if (BCL::rank() == 0)
    {
        for (int i = 0; i < tasks; i += 1)
        {
            queues[0].push(i);
        }
    }

    BCL::barrier();

#pragma omp parallel
{
    while(BCL::rank() == 1 && 1){
        int a;
        while (0 < queues[BCL::rank()].size())
        {
            // printf("%ld %ld A \n", BCL::rank(), omp_get_thread_num());
            queues[BCL::rank()].pop(a, BCL::CircularQueueAL::push_pop);  // <-- here it gets stuck
            // printf("%ld %ld B \n", BCL::rank(), omp_get_thread_num());
        }

        if(extraTasks>0){
                extraTasks--;
                // omp_set_lock(&stealLock);
                queues[1].push(extraTasks, BCL::CircularQueueAL::push_pop);
                // omp_unset_lock(&stealLock);
        }
        else
            break;
    }
}

    BCL::finalize();
}
