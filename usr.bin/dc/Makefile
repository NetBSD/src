#	$NetBSD: Makefile,v 1.6 2025/09/18 03:36:01 mrg Exp $

.include <bsd.own.mk>

PROG=	dc
SRCS=	main.c dc.c bcode.c inout.c mem.c stack.c

WARNS=6
LDADD=	-lcrypto -lpthread
DPADD=	${LIBCRYPTO} ${LIBPTHREAD}

SUBDIR.roff+=USD.doc

COPTS.bcode.c+=	${CC_WNO_CALLOC_TRANSPOSED_ARGS}

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
