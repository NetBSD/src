#	$NetBSD: Makefile,v 1.2 2003/10/11 09:51:49 jdolecek Exp $

PROG=	vinum
SRCS=	v.c list.c vinumparser.c vinumutil.c vext.h commands.c
MAN=	vinum.8

CFLAGS+=	-g
WARNS=	0

DPADD=	${LIBEDIT} ${LIBTERMCAP} ${LIBKVM}
LDADD=	-ledit -ltermcap -lkvm

.PATH: ${.CURDIR}/../../sys/dev/vinum

.include <bsd.prog.mk>
