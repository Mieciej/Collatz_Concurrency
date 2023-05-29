#include<inttypes.h>
#include<stdlib.h>
#include<stdio.h>
#include<time.h>
#include<string.h>
typedef uint64_t natural_number;
natural_number collatz(natural_number x, size_t lim) {
    natural_number steps = 0;
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

void print_array(unsigned long * array, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        printf("%ld ",array[i]);
    }
    printf("\n");
}

int main(int argc, char const *argv[])
{
    if(argc <2)
    {
        exit(0);
    }
    natural_number max_steps = atoi(argv[1]);
    natural_number **results = malloc(sizeof(natural_number*)*(max_steps-11));
    natural_number *ns = malloc(sizeof(size_t)*(max_steps-11));
    for (size_t i = 0; i < max_steps-11; i++)
    {
        ns[i]=0;
        results[i] = malloc(sizeof(natural_number)*5);
    }
    
    natural_number n_results = 0;
    natural_number x=0;
    while (n_results <  (max_steps-11)*5)
    {
        x++;
        printf("Processing %ld\n",x);
        natural_number s = collatz(x,max_steps);
        if(max_steps>s && s>10)
        {
            s-=11;
            if(ns[s]<5)
            {
                results[s][ns[s]++] = x;
                n_results++;
            }
        }
    }
    

    for (natural_number i =0; i < max_steps-11; i++)
    {
        printf("s = %ld\t",i+11);
        print_array(results[i],5);

    }
    return 0;
}
