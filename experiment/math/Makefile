TARGET = vector3d vector_test

all: $(TARGET)

vector_test:	vector_test.o
	$(CC) -o $@ vector3d.o vector_test.o

vector3d:	vector3d.o
	gcc -c -o vector3d.o vector3d.c

clean:
	rm -rf $(TARGET)
	rm -rf *.o
