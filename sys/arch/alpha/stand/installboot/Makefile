# $NetBSD: Makefile,v 1.10 1999/02/13 02:54:37 lukem Exp $

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
