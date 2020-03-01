CC=gcc
INCLUDES=-I./include
LIBS=-lpthread
CFLAGS=-g -Wall -Werror 

SRCDIR=./src/
SRC=file-operations.c multi-lookup.c util.c main.c
SRCS=$(addprefix $(SRCDIR), $(SRC))

OBJS=$(SRCS:.c=.o)

MAIN=multi-lookup

.PHONY: clean

all: $(MAIN)

$(MAIN): $(OBJS)
	echo $(MAIN)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	$(RM) $(MAIN) $(OBJS)
