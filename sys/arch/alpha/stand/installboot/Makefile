# $NetBSD: Makefile,v 1.6 1997/05/18 12:14:55 mycroft Exp $

PROG=	installboot
BINDIR=	/usr/mdec

MAN=	installboot.8
MANSUBDIR=/alpha

CFLAGS+= -g -I${.CURDIR}/../..
LDSTATIC=-static

LDADD+=	-lutil
DPADD+= ${LIBUTIL}

.include <bsd.prog.mk>
