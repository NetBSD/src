#	$NetBSD: Makefile,v 1.14 2001/10/31 21:39:04 thorpej Exp $

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
