#	$NetBSD: Makefile,v 1.4 1999/02/13 02:54:32 lukem Exp $

PROG=	re
SRCS=	main.c split.c debug.c
MKMAN=	no

CPPFLAGS+= -I${.CURDIR}/../../../../lib/libc/regex

TESTS=	${.CURDIR}/tests

regress:
	./re < ${TESTS}
	./re -el < ${TESTS}
	./re -er < ${TESTS}

.include <bsd.prog.mk>
