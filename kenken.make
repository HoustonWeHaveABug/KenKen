KENKEN_C_FLAGS=-c -O2 -Wall -Wextra -Waggregate-return -Wcast-align -Wcast-qual -Wconversion -Wformat=2 -Winline -Wlong-long -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -Wno-import -Wpointer-arith -Wredundant-decls -Wreturn-type -Wshadow -Wstrict-prototypes -Wswitch -Wwrite-strings

kenken: kenken.o
	gcc -o kenken kenken.o

kenken.o: kenken.c kenken.make
	gcc ${KENKEN_C_FLAGS} -o kenken.o kenken.c

clean:
	rm -f kenken kenken.o
