#	$NetBSD: Makefile,v 1.4 2018/06/11 14:18:17 kamil Exp $

.include <bsd.own.mk>

PROG=	dc
SRCS=	main.c dc.c bcode.c inout.c mem.c stack.c

WARNS=6
LDADD=	-lcrypto
DPADD=	${LIBCRYPTO}

SUBDIR.roff+=USD.doc

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
