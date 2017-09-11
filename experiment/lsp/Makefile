SRCS = basic_server.c basic_client.c block_server.c block_client.c
.SUFFIXES = .c .o

SERVER = basic_server.o
CLIENT = basic_client.o
BS = block_server.o
BC = block_client.o
TARGET = basic_server basic_client block_server block_client

all:	$(TARGET)

basic_server:	$(SERVER)
	$(CC) -o basic_server $(SERVER)

basic_client:	$(CLIENT)
	$(CC) -o basic_client $(CLIENT)

block_server:	$(BS)
	$(CC) -o block_server $(BS)

block_client:	$(BC)
	$(CC) -o block_client $(BC)

clean:
	rm -rf $(SERVER) $(CLIENT) $(BS) $(BC) $(TARGET)
