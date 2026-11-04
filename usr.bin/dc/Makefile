#	$NetBSD: Makefile,v 1.4.14.2 2026/11/04 23:28:32 perseant Exp $

.include <bsd.own.mk>

PROG=	dc
SRCS=	main.c dc.c bcode.c inout.c mem.c stack.c

WARNS=6
LDADD=	-lcrypto
DPADD=	${LIBCRYPTO}

SUBDIR.roff+=USD.doc

COPTS.bcode.c+=	${CC_WNO_CALLOC_TRANSPOSED_ARGS}

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
