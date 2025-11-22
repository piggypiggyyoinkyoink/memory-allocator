
all:
	gcc -Wall -Wextra -fPIC -c allocator.c -o allocator.o
	gcc -shared allocator.o -o liballocator.so
	gcc -Wall -Wextra runme.c ./liballocator.so -o runme 

runme:
	gcc -Wall -Wextra runme.c ./liballocator.so -o runme 
test:
	gcc -Wall -Wextra runme.c ./liballocator.so -o runme 
clean:
	rm -rf ./*.o
	rm -rf ./*.so