#	$NetBSD: Makefile,v 1.23 1998/02/19 23:43:37 thorpej Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/17/94

PROG=	savecore
SRCS=	savecore.c zopen.c

MAN=	savecore.8

LDADD=	-lkvm
DPADD=	${LIBKVM}
.PATH:	${.CURDIR}/../../usr.bin/compress

.include <bsd.prog.mk>
