
all: lib runme
	
runme: lib
	gcc -g -Wall -Wextra runme.c ./liballocator.so -o runme -lrt

lib:
	gcc -g -fPIC -c allocator.c -o allocator.o
	gcc -g -shared allocator.o -o liballocator.so
test: lib
	gcc -Wall -Wextra runme.c ./liballocator.so -o runme 
clean:
	rm -rf ./*.o
	rm -rf ./*.so