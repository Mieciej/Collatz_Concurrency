#include<inttypes.h>
#include<stdlib.h>
#include<stdio.h>
#include<time.h>
typedef uint64_t natural_number;
natural_number collatz(natural_number x) {
    natural_number steps = 0;
    while (x != 1) {   
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
    
    natural_number max_steps = 250;
    natural_number results[5];
    time_t start = clock();
    for (natural_number i =11; i < max_steps; i++)
    {

        size_t n_results = 0;
        natural_number x = 1;
        while (n_results <5)
        {
            natural_number steps = collatz(x);
            if(steps == i)
            {
                results[n_results++] = x;  
            }
            x++;
        }
        print_array(results,5);
        
    }
    time_t end = clock();
    time_t final =  end- start;
    printf("%ld \n", final);
    
    return 0;
}
