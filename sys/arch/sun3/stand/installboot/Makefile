#	$NetBSD: Makefile,v 1.15 2001/12/12 01:49:52 tv Exp $

.include <bsd.own.mk>

PROG=	installboot
NOMAN= # defined
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
