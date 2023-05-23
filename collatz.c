#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>

#define CHECK(result, textOnFail)                                              \
  if (((long int)result) == -1) {                                              \
    perror(textOnFail);                                                        \
    exit(1);                                                                   \
}



#define N_THREADS 2
typedef uint64_t natural_number;

typedef struct Data
{
    natural_number MAX_STEPS;
    natural_number eval_steps;
    natural_number last_picked_number;
    natural_number distributed_numbers[N_THREADS];
    size_t proc_id;
    sem_t sem;
} Data;


void print_array(natural_number * array, size_t size);
void * collatz(void * arg) {
    Data * data  = (Data*)arg;
    size_t proc_id;
    sem_wait(&data->sem);
    proc_id = data->proc_id;
    data->proc_id+=1;
    natural_number x = data->distributed_numbers[proc_id]; 
    sem_post(&data->sem);
    int *result = malloc(sizeof(int));
    *result = 0;
    unsigned long steps =0;
    while (x != 1)
    {              
        steps++;
        if (steps > data->eval_steps) return result;  // until you reach 1,
        if (x % 2) x = 3 * x + 1;                   // if the number is odd,
              // multiply it by three and add one,
        else x /= 2;                          // else, if the number is even,
                              // divide it by two
    }
    *result = steps==data->eval_steps;

    return result;
}


int main(int argc, char const *argv[])
{
    int fd = shm_open("/collatz", O_RDWR | O_CREAT , 0666);
    CHECK(fd,"shm_open failed");
    int r = ftruncate(fd, sizeof(Data));
    CHECK(r, "ftruncate failed");
    Data *data = mmap(NULL, sizeof(Data),PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    sem_init(&data->sem,1,1);

    data->eval_steps = 11;
    natural_number results[5] = {0};
    data->MAX_STEPS = 15;
    pthread_t threads[N_THREADS];
    while (data->eval_steps<data->MAX_STEPS)
    {
        /* code */
        data->last_picked_number=3;
        size_t n_of_results = 0;
        while (n_of_results < 5)
        {
            data->proc_id = 0;
            for (size_t i = 0; i < N_THREADS; i++)
            {
                data->distributed_numbers[i]=++data->last_picked_number;
            }
            //print_array(data->distributed_numbers,N_THREADS);

            for (size_t i = 0; i < N_THREADS; i++)
            {
                int p = pthread_create(&threads[i],NULL,collatz,data);
                CHECK(p, "pthread_create failed");
            }
            for (size_t i = 0; i < N_THREADS; i++)
            {
                int *result;
                int p =  pthread_join(threads[i],(void**)&result);
                CHECK(p, "pthread_join failed");

                if(*result)
                {
                    results[n_of_results] = data->distributed_numbers[i];
                    n_of_results++;
                }
                free(result);
            }

        }
        data->eval_steps++;
        print_array(results, 5);
    }

    munmap(data,sizeof(Data));
    
    return 0;
}
    

void print_array(natural_number * array, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        printf("%ld ",array[i]);
    }
    printf("\n");
}