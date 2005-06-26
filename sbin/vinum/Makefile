#	$NetBSD: Makefile,v 1.3 2005/06/26 22:36:55 christos Exp $

PROG=	vinum
SRCS=	v.c list.c vinumparser.c vinumutil.c vext.h commands.c
MAN=	vinum.8

CFLAGS+=	-g

DPADD=	${LIBEDIT} ${LIBTERMCAP} ${LIBKVM}
LDADD=	-ledit -ltermcap -lkvm

.PATH: ${.CURDIR}/../../sys/dev/vinum

.include <bsd.prog.mk>
