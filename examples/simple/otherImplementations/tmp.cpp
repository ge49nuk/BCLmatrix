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

using namespace std;
const int SIZE = 150;

static inline double curtime(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec * 1e-9;
}

void multiply(double *matrix, double *matrix2, double *result);


int main(int argc, char **argv)
{
    

    double s = 0;
    for(int i = 0; i<100; i++){
        
        double a[SIZE*SIZE];
        double b[SIZE*SIZE];
        double c[SIZE*SIZE];
        double g = curtime();
        multiply(a,b,c);
        s+= curtime()-g;
    }
    cout << s << endl;
}

void multiply(double *matrix, double *matrix2, double *result)
{
    // for (int i = 0; i < SIZE * SIZE; i += 1)
    // {
    //     double value = 0;
    //     int k = i % SIZE;
    //     for (int j = (i / SIZE) * SIZE; j < (i / SIZE) * SIZE + SIZE; j++)
    //     {
    //         value = value + matrix[j] * matrix2[k];
    //         k += SIZE;
    //     }
    //     result[i] = value;
    // }
    for(int i=0;i<SIZE;i++) {
		for(int j=0;j<SIZE;j++) {
			result[i*SIZE+j]=0;
			for(int k=0;k<SIZE;k++) {
				result[i*SIZE+j] += matrix[i*SIZE+k] * matrix2[k*SIZE+j];
			}
		}
	}
}

// bool steal(uint16_t my_server_key)
// {
//     srand(unsigned(std::time(0)));
//     vector<uint16_t> ranks;

//     for (uint16_t i = 0; i < comm_size; i += comm_size / ranks_per_server)
//     {
//         if (i != my_rank)
//             ranks.push_back(i);
//     }

//     std::random_shuffle(ranks.begin(), ranks.end());
//     //iterates through ranks and tests if they have tasks left
//     for (std::vector<uint16_t>::iterator it = ranks.begin(); it != ranks.end(); ++it)
//     {

//         //measuring the time until the first response from foreign rank
//         long size = task_queue->Size(*it);
//         cout << "Remote size: " << size << endl;

//         if (size > 0)
//         {
//             //steals half the tasks
//             int j = 0;
//             while (j < task_queue->Size(*it) * 0.5)
//             {
//                 MatTask_Type tmp_pop_T;
//                 printf("Trying to steal %d %d\n", my_rank, *it);
//                 auto popped = task_queue->Pop(*it);
//                 printf("steal %d %d\n", my_rank, *it);
//                 if (popped.first)
//                 {
//                     tmp_pop_T = popped.second;
//                     task_queue->Push(tmp_pop_T, my_server_key);
//                 }
//                 else
//                     break;

//                 j++;
//             }

//             long ownSize = task_queue->Size(my_server_key);
//             if (ownSize > 0)
//             {
//                 printf("[%ld]Successfully stolen %ld/%ld tasks!\n", my_rank, ownSize, size);
//                 return true;
//             }
//         }
//     }

//     return false;
// }
