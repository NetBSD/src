#	$NetBSD: Makefile,v 1.2.4.1 1997/11/08 07:39:38 lukem Exp $

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
