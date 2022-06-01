all: myshell
myshell: shell2.c
	gcc -o myshell shell2.c
clean:
	rm -rf myshell