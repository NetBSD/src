#	$NetBSD: Makefile,v 1.3 1997/01/16 01:20:24 cgd Exp $

.PATH:	${.CURDIR}/..

PROG=	installboot
BINDIR=	/usr/mdec

MAN=	installboot.8
MANSUBDIR=/alpha

CFLAGS+= -g
LDSTATIC=-static

LDADD+=	-lutil
DPADD+= ${LIBUTIL}

.include <bsd.prog.mk>
