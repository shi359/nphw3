Cli=HW3_102062211_Cli.c
Ser=HW3_102062211_Ser.c

all:
	gcc -pthread -o server $(Ser)
	gcc -pthread -o client $(Cli)
clean:
	rm -rf share client server


