#	$NetBSD: Makefile,v 1.3 1997/10/26 21:51:02 lukem Exp $

PROG=	re
SRCS=	main.c split.c debug.c
NOMAN=

CPPFLAGS+= -I${.CURDIR}/../../../../lib/libc/regex

TESTS=	${.CURDIR}/tests

regress:
	./re < ${TESTS}
	./re -el < ${TESTS}
	./re -er < ${TESTS}

.include <bsd.prog.mk>
