cachesim: cachesim.c
	gcc -g -Wall -fsanitize=address -o cachesim cachesim.c -lm

clean:
	rm -f cachesim
