PROG=emuspeed
NOMAN=none

SRCS=emuspeed.c test.s mul64.s div64.s # movepto.c movepfrom.c

/* don't install: */
proginstall:

.include <bsd.prog.mk>

regress: all
	@./emuspeed
