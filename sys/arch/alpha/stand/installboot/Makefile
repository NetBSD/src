# $NetBSD: Makefile,v 1.5 1997/04/06 08:41:11 cgd Exp $

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
