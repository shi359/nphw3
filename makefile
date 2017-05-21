Cli=cli.c
Ser=ser.c

all:
	gcc -o server $(Ser)
	gcc -o client $(Cli)
	./server
clean:
	rm -rf share client server


