
all: lib runme
	
runme: lib
	gcc -Wall -Wextra runme.c ./liballocator.so -o runme 

lib:
	gcc -fPIC -c allocator.c -o allocator.o
	gcc -shared allocator.o -o liballocator.so
test: lib
	gcc -Wall -Wextra runme.c ./liballocator.so -o runme 
clean:
	rm -rf ./*.o
	rm -rf ./*.so