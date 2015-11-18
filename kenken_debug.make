KENKEN_DEBUG_C_FLAGS=-c -O2 -Wall -Wextra -Waggregate-return -Wcast-align -Wcast-qual -Wconversion -Wformat=2 -Winline -Wlong-long -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -Wno-import -Wpointer-arith -Wredundant-decls -Wreturn-type -Wshadow -Wstrict-prototypes -Wswitch -Wwrite-strings

kenken_debug.exe: kenken_debug.o
	gcc -o kenken_debug.exe kenken_debug.o

kenken_debug.o: kenken.c kenken_debug.make
	gcc ${KENKEN_DEBUG_C_FLAGS} -o kenken_debug.o kenken.c
