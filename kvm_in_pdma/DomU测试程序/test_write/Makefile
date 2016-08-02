INCLUDE = tool.h
encode : encode.c tool.o $(INCLUDE)
	gcc -c $(CFLAGS) encode.c -o encode.o
	gcc $(CFLAGS) -o encode encode.o tool.o $(INCLUDE)

tool.o : tool.c
	gcc -c $(CFLAGS) tool.c -o tool.o

clean:
	rm -f encode *.o
