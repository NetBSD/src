#	$NetBSD: Makefile,v 1.3 1999/02/13 02:54:33 lukem Exp $
#
PROG=emuspeed
MKMAN=no

SRCS=emuspeed.c test.s mul64.s div64.s # movepto.c movepfrom.c

/* don't install: */
proginstall:

.include <bsd.prog.mk>

regress: all
	@./emuspeed
