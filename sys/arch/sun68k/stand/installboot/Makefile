#	$NetBSD: Makefile,v 1.1.6.1 2001/11/12 21:17:38 thorpej Exp $

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

SRCS=	installboot.c byteorder.c loadfile.c loadfile_aout.c loadfile_elf32.c

.include <bsd.prog.mk>
