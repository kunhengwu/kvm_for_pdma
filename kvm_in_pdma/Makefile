CC=gcc
TAR=host
SRC=host.c queue.c
KERNEL_INCLUDE ?= /lib/modules/$(shell uname -r)/build/include
all:
	$(CC) -D__KERNEL__ -o $(TAR) -I$(KERNEL_INCLUDE) -g $(SRC) -lpthread

clean: 
	rm -f $(TAR)