CC = gcc
CFLAGS = -g -Wall -Werror
OS = $(shell uname -s)
PROC = $(shell uname -p)
EXEC_SUFFIX=$(OS)-$(PROC)

all:  json-server-$(EXEC_SUFFIX)

trace-$(EXEC_SUFFIX): trace.c
	$(CC) $(CFLAGS) -o $@ json-server.c

clean:
	-rm -rf json-server-*
