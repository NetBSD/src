#	$NetBSD: Makefile,v 1.8.4.1 1999/06/21 01:01:56 thorpej Exp $

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
