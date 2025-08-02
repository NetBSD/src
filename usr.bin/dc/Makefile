#	$NetBSD: Makefile,v 1.4.14.1 2025/08/02 05:58:25 perseant Exp $

.include <bsd.own.mk>

PROG=	dc
SRCS=	main.c dc.c bcode.c inout.c mem.c stack.c

WARNS=6
LDADD=	-lcrypto -lpthread
DPADD=	${LIBCRYPTO} ${LIBPTHREAD}

SUBDIR.roff+=USD.doc

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
