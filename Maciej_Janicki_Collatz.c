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
#include<signal.h>

void print_array(unsigned long * array, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        printf("%ld ",array[i]);
    }
    printf("\n");
}

typedef struct
{
    sem_t sem;
    size_t n;
    unsigned long result[5];

}Entry;

typedef struct 
{   
    unsigned long max_steps;
    unsigned long last_x;
    unsigned long smallest_x;
    unsigned long n_results;
    sem_t sem_last_x;
    sem_t sem_new_result;
    Entry entries[];
}Data;

unsigned long collatz(unsigned long x, size_t lim) {
    unsigned long steps = 0;
    while (x != 1) {
        steps++;              // until you reach 1,
        if(steps >lim) break;   
        if (x % 2)                     // if the number is odd,
            x = 3 * x + 1;               // multiply it by three and add one,
        else                           // else, if the number is even,
            x /= 2;                      // divide it by two
  }
  return steps ;
}
volatile sig_atomic_t stopFlag = 0;
void ctrlC(int num)
{
     stopFlag = 1; 
}

int main(int argc, char const *argv[])
{
    if(argc < 2)
    {
        fprintf(stderr,"Too few arguments! \n");
        exit(0);

    }
    unsigned long max_steps;
    max_steps = atoi(argv[1]);

    if(max_steps <= 10)
    {
        fprintf(stderr,"MAX SIZE too small! \n");
        exit(1);
    }
    signal(SIGINT,ctrlC);        
    size_t data_size = sizeof(unsigned long)*4+sizeof(sem_t)*2+(max_steps-11)*sizeof(Entry);
    int fd = shm_open("/collatz_results", O_RDWR | O_CREAT , 0666);
    off_t fsize;
    fsize = lseek(fd, 0, SEEK_END);
    if(fsize<data_size)
        ftruncate(fd, data_size);
    lseek(fd,0,SEEK_SET);

    Data* data = mmap(NULL, data_size,PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    
    if (data->max_steps==0)
    {
        data->max_steps=max_steps;
        for (size_t i = 0; i < max_steps-11; i++)
        {
            sem_init(&data->entries[i].sem,1,1);
        }
        sem_init(&data->sem_new_result,1,1);
        sem_init(&data->sem_last_x,1,1);     
        data->last_x = 0;
        data->smallest_x = 2;
        data->n_results = 0; 
    }
    else if(data->max_steps < max_steps)
    {
        fprintf(stderr,"MAX SIZE too big! \n");
        exit(1);
        
    }
    
    while (!stopFlag)
    {
        unsigned long x;
        sem_wait(&data->sem_last_x);
        x = ++data->last_x;
        sem_post(&data->sem_last_x);
        sem_wait(&data->sem_new_result);
        unsigned long small  = data->smallest_x;
        unsigned long n_results = data->n_results;
        sem_post(&data->sem_new_result);
        if(n_results>=(max_steps-11)*5 && small <= x) break;


        //printf("Processing %ld\n",x);
        unsigned long s = collatz(x,max_steps);
        if(s >= max_steps || s<=10) continue;
        s-=11;
        sem_wait(&data->entries[s].sem);

        if(data->entries[s].n < 5) 
        {
            data->entries[s].result[data->entries[s].n++]=x;
            sem_wait(&data->sem_new_result);
            data->n_results++;
            if(data->smallest_x>x)
            {
                data->smallest_x = x;
            }
            sem_post(&data->sem_new_result);
        }
        else
        {
            size_t max_pos = 0;
            for (size_t i = 1; i < 5; i++)
            {
                if(data->entries[s].result[max_pos]<data->entries[s].result[i])
                {
                    max_pos = i;
                }
            }
            if(data->entries[s].result[max_pos]>x)
            {
                data->entries[s].result[max_pos] = x;
                sem_wait(&data->sem_new_result);
                if(data->smallest_x>x)
                {
                    data->smallest_x = x;
                }
                sem_post(&data->sem_new_result);
            }
        }
        sem_post(&data->entries[s].sem);
        
    
    }
    if(stopFlag)
    {
        munmap(data,data_size);
        exit(0);
    }
    for (size_t i = 0; i < max_steps-11; i++)
    {
        printf("s = %ld\t",i+11);
        sem_wait(&data->entries[i].sem);
        print_array(data->entries[i].result,5);
        sem_post(&data->entries[i].sem);
    }

    munmap(data,data_size);
    return 0;
}
