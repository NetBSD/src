# $NetBSD: Makefile,v 1.9 1998/03/02 19:57:00 cgd Exp $

.include <bsd.own.mk>

PROG=	installboot
BINDIR=	/usr/mdec

WARNS?=	1

.PATH:	${.CURDIR}/../../../../../sbin/disklabel

SRCS=	installboot.c cd9660.c dkcksum.c

NOMAN=	yes

CFLAGS+= -g -I${.CURDIR}/../..
LDSTATIC?=-static

LDADD+=	-lutil
DPADD+= ${LIBUTIL}

.include <bsd.prog.mk>
