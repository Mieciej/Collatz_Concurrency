#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include<time.h>
#define N_THREADS 2

#define CHECK(result, textOnFail)                                              \
  if (((long int)result) == -1) {                                              \
    perror(textOnFail);                                                        \
    exit(1);                                                                   \
}

void print_array(unsigned long * array, size_t size);


unsigned long collatz(unsigned long x)
{
    unsigned long steps = 0;
    while (x != 1)
    {
        steps++;           // until you reach 1,
        if (x % 2)         // if the number is odd,
            x = 3 * x + 1; // multiply it by three and add one,
        else               // else, if the number is even,
            x /= 2;        // divide it by two
    }
    return steps;
}

typedef struct
{
    unsigned long x [N_THREADS];
    unsigned long curr_steps;
    unsigned long max_steps;
    unsigned long n_results;
    unsigned long results[5];
    // sem_t x_sem_r; 
    // sem_t x_sem_w;
    sem_t x_sem;
    sem_t result_sem;
    //size_t n_readers;
    
} Data;

Data *data;

void *thread(void *n_thrd)
{
    size_t n = *(size_t *)n_thrd;
    Data *local =data;

    while (data->n_results<5)
    {
        unsigned long steps = collatz(data->x[n]);
        if(steps == data->curr_steps)
        {
            unsigned long min =0;
            while (data->x[n] > min && data->n_results < 5)
            {
                sem_wait(&data->x_sem);
                // sem_wait(&data->x_sem_r);
                // data->n_readers +=1;
                // if (data->n_readers == 1) sem_wait(&data->x_sem_w);
                // sem_post(&data->x_sem_r);
                min = data->x[0];
                for (size_t i = 1; i < N_THREADS; i++)
                {
                    if (min> data->x[i])
                    {
                        min = data->x[i];
                    }
                }
                sem_post(&data->x_sem);

                // sem_wait(&data->x_sem_r);
                // data->n_readers -=1;
                // if (data->n_readers == 0) sem_post(&data->x_sem_w);
                // sem_post(&data->x_sem_r);
            }
            sem_wait(&data->result_sem);
            if(data->n_results>=5)
            {
                sem_post(&data->result_sem);
                return NULL;
            }
            data->results[data->n_results] = data->x[n];
            data->n_results++;
            sem_post(&data->result_sem);

            
        }
        // sem_wait(&data->x_sem_w);
        sem_wait(&data->x_sem);
        data->x[n]+=N_THREADS;
        sem_post(&data->x_sem);

        // sem_post(&data->x_sem_w);
    }
    
    return NULL;
}

int main(int argc, char const *argv[])
{
    size_t n_thrds[N_THREADS];
    pthread_t threads[N_THREADS];
    int fd = shm_open("/collatz", O_RDWR | O_CREAT , 0666);
    CHECK(fd,"shm_open failed");
    int r = ftruncate(fd, sizeof(Data));
    CHECK(r, "ftruncate failed");
    data = mmap(NULL, sizeof(Data),PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    Data * local_data =data;

    for (size_t i = 0; i < 5; i++)
    {
        data->results[i]=0;
    }
    
    data->curr_steps=11;
    data->max_steps = 12;
    //data->n_readers = 0;
    time_t start = clock();
    while (data->curr_steps < data->max_steps)
    {
        // sem_init(&data->x_sem_r,1,1);
        // sem_init(&data->x_sem_w,1,1);
        sem_init(&data->x_sem,1,1);
        sem_init(&data->result_sem,1,1);
        data->n_results = 0;
        for (size_t i = 0; i < N_THREADS; i++)
        {
            n_thrds[i] = i;
            data->x[i] = i+1;
        }
        for (size_t i = 0; i < N_THREADS; i++)
        {
            pthread_create(threads+i,NULL,thread,n_thrds+i);
        }
        for (size_t i = 0; i < N_THREADS; i++)
        {
            pthread_join(threads[i],NULL);
        }
        print_array(data->results,5);
        data->curr_steps++;

    }
    time_t end = clock();
    time_t final =  end - start;
    printf("%ld \n", final);
    munmap(data,sizeof(Data));
    

    return 0;
}

void print_array(unsigned long * array, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        printf("%ld ",array[i]);
    }
    printf("\n");
}