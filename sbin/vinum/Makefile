#	$NetBSD: Makefile,v 1.1.1.1 2003/10/10 03:13:12 grog Exp $

PROG=	vinum
SRCS=	v.c list.c vinumparser.c vinumutil.c vext.h commands.c
MAN=	vinum.8

CFLAGS+=	-g
WARNS=	0

DPADD=	${LIBREADLINE} ${LIBTERMCAP} ${LIBDEVSTAT} ${LIBKVM}
LDADD=	  -ledit -ltermcap -lkvm

.PATH: ${.CURDIR}/../../sys/dev/vinum

.include <bsd.prog.mk>
