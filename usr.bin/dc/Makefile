#	$NetBSD: Makefile,v 1.5 2025/07/19 12:21:52 christos Exp $

.include <bsd.own.mk>

PROG=	dc
SRCS=	main.c dc.c bcode.c inout.c mem.c stack.c

WARNS=6
LDADD=	-lcrypto -lpthread
DPADD=	${LIBCRYPTO} ${LIBPTHREAD}

SUBDIR.roff+=USD.doc

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
