# $NetBSD: Makefile,v 1.8 1998/02/01 06:59:30 thorpej Exp $

PROG=	installboot
BINDIR=	/usr/mdec

WARNS?=	1

.PATH:	${.CURDIR}/../../../../../sbin/disklabel

SRCS=	installboot.c cd9660.c dkcksum.c

NOMAN=	yes

CFLAGS+= -g -I${.CURDIR}/../..
LDSTATIC=-static

LDADD+=	-lutil
DPADD+= ${LIBUTIL}

.include <bsd.prog.mk>
