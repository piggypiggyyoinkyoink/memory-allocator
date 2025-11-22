
all:
	gcc -Wall -Wextra -fPIC -c allocator.c -o allocator.o
	gcc -shared allocator.o -o allocator.so
	gcc -Wall -Wextra runme.c ./allocator.so -o runme 


runme:
	gcc -Wall -Wextra runme.c ./allocator.so -o runme 

clean:
	rm -rf ./*.o