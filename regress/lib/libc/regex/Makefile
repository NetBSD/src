PROG=re
SRCS=main.c split.c debug.c
NOMAN=

TESTS=/usr/src/lib/libc/regex/tests

regress:
	./re < ${TESTS}
	./re -el < ${TESTS}
	./re -er < ${TESTS}

.include <bsd.prog.mk>
.PATH: /usr/src/lib/libc/regex
