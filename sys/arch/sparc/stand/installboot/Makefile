#	$NetBSD: Makefile,v 1.9 1999/04/28 15:22:25 christos Exp $

.include <bsd.own.mk>

WARNS?=1
PROG=	installboot
SRCS=	installboot.c loadfile.c
MKMAN=	no
CFLAGS+=-g

LIBSA=${.CURDIR}/../../../../lib/libsa

.PATH.c: ${LIBSA}

CPPFLAGS+=-I${LIBSA}

LDSTATIC?=	-static

.include <bsd.prog.mk>
