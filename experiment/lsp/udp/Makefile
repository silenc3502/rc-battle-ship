TARGET = udp_server udp_client

all: $(TARGET)

udp_server: udp_server.o
	$(CC) -o $@ udp_server.o

udp_client: udp_client.o
	$(CC) -o $@ udp_client.o

clean:
	rm -rf $(TARGET)
	rm -rf *.o
