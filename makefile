Cli=HW3_102062211_Cli.c
Ser=HW3_102062211_Ser.c

all:
	gcc -o server $(Ser)
	gcc -o client $(Cli)
clean:
	rm -rf share client server


