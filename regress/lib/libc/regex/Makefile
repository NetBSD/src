#	$NetBSD: Makefile,v 1.5 2001/12/12 01:24:03 tv Exp $

PROG=		re
SRCS=		main.c split.c debug.c
NOMAN=		# defined
CPPFLAGS+=	-I${.CURDIR}/../../../../lib/libc/regex

TESTS=		${.CURDIR}/tests

regress:
	./re < ${TESTS}
	./re -el < ${TESTS}
	./re -er < ${TESTS}

.include <bsd.prog.mk>
