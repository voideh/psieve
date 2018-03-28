main:
	gcc -Wall -O0 -pthread -g sieve.c -o sieve -lm

clean:
	rm sieve
