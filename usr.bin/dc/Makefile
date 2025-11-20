#	$NetBSD: Makefile,v 1.5.2.1 2025/11/20 18:46:43 martin Exp $

.include <bsd.own.mk>

PROG=	dc
SRCS=	main.c dc.c bcode.c inout.c mem.c stack.c

WARNS=6
LDADD=	-lcrypto
DPADD=	${LIBCRYPTO}

SUBDIR.roff+=USD.doc

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
