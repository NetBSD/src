#	$NetBSD: Makefile,v 1.4 1997/01/24 01:53:00 cgd Exp $

.PATH:	${.CURDIR}/..

PROG=	installboot
BINDIR=	/usr/mdec

MAN=	installboot.8
MANSUBDIR=/alpha

CFLAGS+= -g -I${.CURDIR}/../..
LDSTATIC=-static

LDADD+=	-lutil
DPADD+= ${LIBUTIL}

.include <bsd.prog.mk>
