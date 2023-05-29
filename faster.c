#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include<time.h>
#include<string.h>
#include<stdatomic.h>
#define N_THREADS 8

#define CHECK(result, textOnFail)                                              \
  if (((long int)result) == -1) {                                              \
    perror(textOnFail);                                                        \
    exit(1);                                                                   \
}

void print_array(unsigned long * array, size_t size);
void swap (unsigned long *xp, unsigned long *yp);

unsigned long collatz(unsigned long x, size_t lim) {
    unsigned long steps = 0;
    while (x != 1) {
        if(steps >lim) break;   
        steps++;              // until you reach 1,
        if (x % 2)                     // if the number is odd,
            x = 3 * x + 1;               // multiply it by three and add one,
        else                           // else, if the number is even,
            x /= 2;                      // divide it by two
  }
  return steps;
}

typedef struct
{
    unsigned long tested_s[N_THREADS];
    unsigned long results[N_THREADS][5];
    unsigned long max_steps;
    size_t num_done;
    sem_t add_result;
    atomic_bool all_done;
    sem_t wait_for_printing[N_THREADS];
    
} Data;

Data *data;

void *thread(void *arg)
{
    int n = *(int *) arg;
    unsigned long *results = malloc(sizeof(unsigned long)* 5);

    while (data->tested_s[n] < data->max_steps)
    {

        size_t n_results = 0;
        unsigned long x = 1;
        while (n_results <5)
        {
            unsigned long steps = collatz(x,data->tested_s[n]);
            if(steps == data->tested_s[n])
            {
                results[n_results++] = x;  
            }
            x++;
        }
        sem_wait(&data->add_result);
        memcpy(data->results[n],results,sizeof(unsigned long)* 5);
        int b = ++data->num_done==N_THREADS;
        atomic_store(&data->all_done,b );
        sem_post(&data->add_result);
        sem_wait(&data->wait_for_printing[n]);
                
    }
    sem_wait(&data->add_result);
    int b = ++data->num_done==N_THREADS;
    atomic_store(&data->all_done,b );
    sem_post(&data->add_result);

    
    
    
    return NULL;
}

int main(int argc, char const *argv[])
{
    if(argc <2)
    {
        exit(0);
    }

    size_t n_thrds[N_THREADS];
    pthread_t threads[N_THREADS];
    int fd = shm_open("/collatz", O_RDWR | O_CREAT , 0666);
    CHECK(fd,"shm_open failed");
    int r = ftruncate(fd, sizeof(Data));
    CHECK(r, "ftruncate failed");
    data = mmap(NULL, sizeof(Data),PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    for (size_t i = 0; i < N_THREADS; i++)
    {
        data->tested_s[i]=11+i;
    }

    data->max_steps = atoi(argv[1]);
    
    sem_init(&data->add_result,1,1);

    for (size_t i = 0; i < N_THREADS; i++)
    {
        sem_init(&data->wait_for_printing[i],1,0);
    }
    for (size_t i = 0; i < N_THREADS; i++)
    {
        n_thrds[i] =i;
        pthread_create(threads+i,NULL,thread,n_thrds+i);   
    }
    
    
    for (size_t j = 10; j <data->max_steps;j+=N_THREADS )
    {   
        data->num_done = 0;
        atomic_store(&data->all_done,0);
        for (size_t i = 0; i < N_THREADS; i++)
        {
            sem_post(&data->wait_for_printing[i]);
        }
        
        while (!atomic_load(&data->all_done));
        
        for (size_t i = 0; i < (data->max_steps-j>N_THREADS ? N_THREADS : data->max_steps-j -1); i++)
        {
            printf("s = %ld: ",data->tested_s[i]);
            print_array(data->results[i],5);
        }
        for (size_t i = 0; i < N_THREADS; i++)
        {
            data->tested_s[i]+=N_THREADS;
        }
        
    }
    for (size_t i = 0; i < N_THREADS; i++)
    {
        sem_post(&data->wait_for_printing[i]);
    }
    for (size_t i = 0; i < N_THREADS; i++)
    {
        pthread_join(threads[i],NULL);
    }
    



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
