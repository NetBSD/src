#	$NetBSD: Makefile,v 1.3 2017/05/21 15:28:43 riastradh Exp $

.include <bsd.own.mk>

PROG=	dc
SRCS=	main.c dc.c bcode.c inout.c mem.c stack.c

WARNS=6
LDADD=	-lcrypto
DPADD=	${LIBCRYPTO}

SUBDIR+=USD.doc

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
