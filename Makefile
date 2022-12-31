# First target is default target, if you just type:  make

FILE=myshell.c

default: run

run: myshell
	./myshell

build: myshell

check: run

gdb: myshell
	gdb --args myshell

myshell: ${FILE}
	gcc -g -O0 -o myshell ${FILE}

emacs: ${FILE}
	emacs ${FILE}
vi: ${FILE}
	vi ${FILE}

clean:
	rm -f myshell a.out *~
