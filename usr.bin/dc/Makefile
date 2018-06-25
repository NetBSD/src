#	$NetBSD: Makefile,v 1.3.8.1 2018/06/25 07:26:10 pgoyette Exp $

.include <bsd.own.mk>

PROG=	dc
SRCS=	main.c dc.c bcode.c inout.c mem.c stack.c

WARNS=6
LDADD=	-lcrypto
DPADD=	${LIBCRYPTO}

SUBDIR.roff+=USD.doc

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
