#	$NetBSD: Makefile,v 1.12 2001/03/21 13:43:59 tsutsui Exp $

.include <bsd.own.mk>

PROG=	installboot
MKMAN= no
BINDIR=/usr/mdec
S= ${.CURDIR}/../../../..
LIBSA=${S}/lib/libsa
CPPFLAGS+= -I${LIBSA} -I.
WARNS?= 1

# Need this to work in the miniroot
LDSTATIC?=	-static

.PATH.c: ${LIBSA}

SRCS=	installboot.c loadfile.c

.include <bsd.prog.mk>
