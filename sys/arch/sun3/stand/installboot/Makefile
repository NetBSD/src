#	$NetBSD: Makefile,v 1.7.8.3 2001/03/27 15:31:38 bouyer Exp $

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
