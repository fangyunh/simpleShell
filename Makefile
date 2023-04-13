all: sshell.o sshell

sshell.o: sshell.c
	gcc -Wall -Wextra -Werror -c sshell.c -o sshell.o

sshell: sshell.o
	gcc -Wall -Wextra -Werror sshell.o -o sshell

clean:
	rm -f *.o sshell