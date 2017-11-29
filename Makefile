CC = gcc
CFLAGS = -g -Wall -Werror
OS = $(shell uname -s)
PROC = $(shell uname -p)
EXEC_SUFFIX=$(OS)-$(PROC)

all:  json-server-$(EXEC_SUFFIX)

json-server-$(EXEC_SUFFIX): json-server.c
	$(CC) $(CFLAGS) -o $@ smartalloc.c json-server.c

clean:
	-rm -rf json-server-*

handin:
	handin bellardo 464_fp Makefile README smartalloc.c smartalloc.h json-server.c json-server.h
