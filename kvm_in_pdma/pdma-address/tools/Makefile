

all: pdma-read pdma-read1 pdma-read2 pdma-read3 pdma-read4 pdma-write pdma-dma-start pdma-dma-stop pdma-info pdma-rw-reg pdma-write1 pdma-write2 pdma-write3 pdma-write4 pdma-write5 ad9361_config

#vpath %.h ../
#VPATH=../

INCLUDE=pdma-lib.h ../pdma-ioctl.h

pdma-read: pdma-read.c pdma-lib.o $(INCLUDE)
	gcc -c $(CFLAGS) pdma-read.c -o pdma-read.o
	gcc $(CFLAGS) -o pdma-read pdma-read.o pdma-lib.o $(INCLUDE)

pdma-read1: pdma-read1.c pdma-lib.o $(INCLUDE)
	gcc -c $(CFLAGS) pdma-read1.c -o pdma-read1.o
	gcc $(CFLAGS) -o pdma-read1 pdma-read1.o pdma-lib.o $(INCLUDE)

pdma-read2: pdma-read2.c pdma-lib.o $(INCLUDE)
	gcc -c $(CFLAGS) pdma-read2.c -o pdma-read2.o
	gcc $(CFLAGS) -o pdma-read2 pdma-read2.o pdma-lib.o $(INCLUDE)

pdma-read3: pdma-read3.c pdma-lib.o $(INCLUDE)
	gcc -c $(CFLAGS) pdma-read3.c -o pdma-read3.o
	gcc $(CFLAGS) -o pdma-read3 pdma-read3.o pdma-lib.o $(INCLUDE)

pdma-read4: pdma-read4.c pdma-lib.o $(INCLUDE)
	gcc -c $(CFLAGS) pdma-read4.c -o pdma-read4.o
	gcc $(CFLAGS) -o pdma-read4 pdma-read4.o pdma-lib.o $(INCLUDE)

pdma-write: pdma-write.c pdma-lib.o $(INCLUDE)
	gcc -c $(CFLAGS) pdma-write.c -o pdma-write.o
	gcc $(CFLAGS) -o pdma-write pdma-write.o pdma-lib.o $(INCLUDE)

pdma-write1: pdma-write1.c pdma-lib.o $(INCLUDE)
	gcc -c $(CFLAGS) pdma-write1.c -o pdma-write1.o -lm 
	gcc $(CFLAGS) -o pdma-write1  pdma-write1.o pdma-lib.o $(INCLUDE) -lm

pdma-write2: pdma-write2.c pdma-lib.o $(INCLUDE)
	gcc -c $(CFLAGS) pdma-write2.c -o pdma-write2.o -lm 
	gcc $(CFLAGS) -o pdma-write2  pdma-write2.o pdma-lib.o $(INCLUDE) -lm

pdma-write3: pdma-write3.c pdma-lib.o $(INCLUDE)
	gcc -c $(CFLAGS) pdma-write3.c -o pdma-write3.o -lm 
	gcc $(CFLAGS) -o pdma-write3  pdma-write3.o pdma-lib.o $(INCLUDE) -lm

pdma-write4: pdma-write4.c pdma-lib.o $(INCLUDE)
	gcc -c $(CFLAGS) pdma-write4.c -o pdma-write4.o -lm 
	gcc $(CFLAGS) -o pdma-write4  pdma-write4.o pdma-lib.o $(INCLUDE) -lm

pdma-write5: pdma-write5.c pdma-lib.o $(INCLUDE)
	gcc -c $(CFLAGS) pdma-write5.c -o pdma-write5.o -lm 
	gcc $(CFLAGS) -o pdma-write5  pdma-write5.o pdma-lib.o $(INCLUDE) -lm

pdma-dma-start: pdma-dma-start.c pdma-lib.o $(INCLUDE)
	gcc -c $(CFLAGS) pdma-dma-start.c -o pdma-dma-start.o
	gcc $(CFLAGS) -o pdma-dma-start pdma-dma-start.o pdma-lib.o $(INCLUDE)

pdma-dma-stop: pdma-dma-stop.c pdma-lib.o $(INCLUDE)
	gcc -c $(CFLAGS) pdma-dma-stop.c -o pdma-dma-stop.o
	gcc $(CFLAGS) -o pdma-dma-stop pdma-dma-stop.o pdma-lib.o $(INCLUDE)

pdma-info: pdma-info.c pdma-lib.o $(INCLUDE)
	gcc -c $(CFLAGS) pdma-info.c -o pdma-info.o
	gcc $(CFLAGS) -o pdma-info pdma-info.o pdma-lib.o $(INCLUDE)

pdma-rw-reg: pdma-rw-reg.c pdma-lib.o $(INCLUDE)
	gcc -c $(CFLAGS) pdma-rw-reg.c -o pdma-rw-reg.o
	gcc $(CFLAGS) -o pdma-rw-reg pdma-rw-reg.o pdma-lib.o $(INCLUDE)

ad9361_config: pdma-rw-reg.c pdma-lib.o $(INCLUDE)
	gcc -c $(CFLAGS) ad9361_config.c -o ad9361_config.o
	gcc $(CFLAGS) -o ad9361_config ad9361_config.o pdma-lib.o $(INCLUDE)

pdma-lib.o: pdma-lib.c
		gcc -c $(CFLAGS) pdma-lib.c -o pdma-lib.o

clean:
	rm -f pdma-read pdma-read1 pdma-read2 pdma-read3 pdma-read4 pdma-write pdma-write1 pdma-write2 pdma-write3 pdma-write4 pdma-write5 pdma-dma-stop pdma-dma-start pdma-info ad9361_config\
	pdma-rw-reg *.o

.PHONY: all clean
