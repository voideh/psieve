main: queue.o
	gcc -Wall -O0 -pthread -g queue.o sieve.c -o sieve -lm
seq:
	gcc -Wall seq_sieve.c -o seq_sieve -lm
queue.o:
	gcc -c -Wall -pthread queue.c
clean:
	rm sieve *.o
