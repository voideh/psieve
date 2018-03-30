#include <stdio.h>
#include <math.h>
#include <stdlib.h>


typedef struct {
    int value;
    int is_prime;
} prime_t;

int main(int argc, char** argv)
{
    if(argc != 2)
    {
        fprintf(stderr, "usage: seq_sieve [max_number]\n");
        return 1;
    }
    int max_number = atoi(argv[1]);
    prime_t** primes = (prime_t**)malloc(sizeof(prime_t*) * max_number);

    for(int i = 0; i < max_number; i++)
    {
        primes[i] = (prime_t*)malloc(sizeof(prime_t));
        primes[i]->value = i+1;
        primes[i]->is_prime = 1;
    }

    int check_till = (int)sqrt(max_number);
    for(int i = 2; i <= check_till; i++)
    {
        if(primes[i-1]->is_prime)
        {
            for(int j = i*i; j <= max_number; j+= i)
            {
                primes[j-1]->is_prime = 0;
            }
        }
    }
    

    printf("%d.is_prime = %d\n", primes[max_number-1]->value, primes[max_number-1]->is_prime);
    for(int i = 0; i < max_number; i++)
        free(primes[i]);
    free(primes);

    return 0;
}
