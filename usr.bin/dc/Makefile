#	$NetBSD: Makefile,v 1.7 2025/10/19 01:00:00 riastradh Exp $

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
