all : test

test : myshell.c
		gcc  myshell.c -o myshell -lreadline

clean :
		rm myshell
