#	$NetBSD: Makefile,v 1.25 2001/06/13 23:16:27 wiz Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/17/94

PROG=	savecore
SRCS=	savecore.c zopen.c

MAN=	savecore.8

LDADD=	-lkvm -lutil -lz
DPADD=	${LIBKVM} ${LIBUTIL} ${LIBZ}

.include <bsd.prog.mk>
