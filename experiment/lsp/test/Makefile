TARGET = mp_server basic_client epoll_lt_serv client

all: $(TARGET)

mp_server: mp_server.o
	$(CC) -o $@ mp_server.o

basic_client: basic_client.o
	$(CC) -o $@ basic_client.o

epoll_lt_serv: epoll_lt_serv.o
	$(CC) -o $@ epoll_lt_serv.o

client: client.o
	$(CC) -o $@ client.o

clean:
	rm -rf $(TARGET)
	rm -rf *.o
