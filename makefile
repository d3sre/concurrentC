CC      = gcc
CFLAGS  = 
LDFLAGS = -lpthread
SOURCES_SERVER = grabFirstServer.c grabProtocolTranslator.c SimpleLogger.c
SOURCES_CLIENT = grabFirstClient.c grabProtocolTranslator.c SimpleLogger.c
OBJECTS_SERVER = $(SOURCES_SERVER:.c=.o)
OBJECTS_CLIENT = $(SOURCES_CLIENT:.c=.o)
EXECUTABLE_SERVER = server
EXECUTABLE_CLIENT = client

all: server client

server: $(OBJECTS_SERVER)
	$(CC) $(CFLAGS) $(OBJECTS_SERVER) $(LDFLAGS) -o $(EXECUTABLE_SERVER) 

client: $(OBJECTS_CLIENT)
	$(CC) $(CFLAGS) $(OBJECTS_CLIENT) $(LDFLAGS) -o $(EXECUTABLE_CLIENT) 

%.o: %.c
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean: clean-server clean-client

.PHONY: clean-server
clean-server:
	rm -rf $(EXECUTABLE_SERVER) $(OBJECTS_SERVER)

.PHONY: clean-client
clean-client:
	rm -rf $(EXECUTABLE_CLIENT) $(OBJECTS_CLIENT)