#	$NetBSD: Makefile,v 1.6 2002/09/18 04:16:01 lukem Exp $

NOMAN=		# defined

.include <bsd.own.mk>

PROG=		re
SRCS=		main.c split.c debug.c
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libc/regex

TESTS=		${.CURDIR}/tests

regress:
	./re < ${TESTS}
	./re -el < ${TESTS}
	./re -er < ${TESTS}

.include <bsd.prog.mk>
