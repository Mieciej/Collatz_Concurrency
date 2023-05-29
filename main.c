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

typedef enum { NOT_PROCESSED = 0,BEING_PROCESSED = 1, PROCESSED = 2} Status;

typedef struct
{
    unsigned long result[5];
    Status state;
}Entry;

typedef struct 
{   
    unsigned long max_steps;
    sem_t sem_ent;
    Entry entries[];
}Data;

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
volatile sig_atomic_t stopFlag = 0;
void ctrlC(int num)
{
     stopFlag = 1; 
}
void print_array(unsigned long * array, size_t size);
int main(int argc, char const *argv[])
{
    if(argc < 2)
    {
        fprintf(stderr,"Too few arguments! \n");
        exit(0);

    }
    signal(SIGINT,ctrlC);        
    unsigned long max_steps;
    max_steps = atoi(argv[1]);
    if(max_steps <= 10)
    {
        fprintf(stderr,"MAX SIZE too small! \n");
        exit(1);
    }
    size_t data_size = sizeof(unsigned long)+(sizeof(sem_t))+(max_steps-11)*(sizeof(Entry));
    int fd = shm_open("/results", O_RDWR | O_CREAT , 0666);
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
        sem_init(&data->sem_ent,1,1);
    }
    else if(data->max_steps < max_steps)
    {
        fprintf(stderr,"MAX SIZE too big! \n");
        exit(1);
        
    }
    
    for (size_t i = 0; i < max_steps-11 && !stopFlag; i++)
    {
        int processing = 0;
        sem_wait(&data->sem_ent);
        processing= data->entries[i].state==NOT_PROCESSED;
        if(processing)
        {
            data->entries[i].state = BEING_PROCESSED;
        }
        sem_post(&data->sem_ent);
        if(processing)
        {
            size_t result_size =0;
            unsigned long x=1;
            unsigned long target_steps=i+11;
            printf("Processing: %ld \n",target_steps);
            while (result_size<5)
            {
                if(target_steps==collatz(x,target_steps))
                {
                    data->entries[i].result[result_size++] = x;
                }
                x++;
            }
            sem_wait(&data->sem_ent);
            data->entries[i].state = PROCESSED;
            sem_post(&data->sem_ent);
            
        }
        
    }
    if(stopFlag)
    {
        printf("Stopping...\n");
        exit(EXIT_SUCCESS);
    }
    for (size_t i = 0; i < max_steps-11; i++)
    {
        while (data->entries[i].state!=PROCESSED);
        
        print_array(data->entries[i].result,5);
    }
    munmap(data,data_size);
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
