# $NetBSD: Makefile,v 1.11 1999/04/05 03:02:00 cgd Exp $

.include <bsd.own.mk>

PROG=	installboot
BINDIR=	/usr/mdec

WARNS?=	1

.PATH:	${.CURDIR}/../../../../../sbin/disklabel

SRCS=	installboot.c cd9660.c dkcksum.c

MKMAN=	no

CFLAGS+= -g -I${.CURDIR}/../..
LDSTATIC?=-static

LDADD+=	-lutil
DPADD+= ${LIBUTIL}

.include <bsd.prog.mk>
