# $NetBSD: Makefile,v 1.7.6.2 1997/06/22 05:09:56 mrg Exp $

PROG=	installboot
BINDIR=	/usr/mdec

NOMAN=	yes

CFLAGS+= -g -I${.CURDIR}/../..
LDSTATIC=-static

LDADD+=	-lutil
DPADD+= ${LIBUTIL}

.include <bsd.prog.mk>
