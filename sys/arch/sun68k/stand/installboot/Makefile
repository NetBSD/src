#	$NetBSD: Makefile,v 1.1.2.1 2002/01/10 19:49:54 thorpej Exp $

NOMAN= # defined

.include <bsd.own.mk>

PROG=	installboot
BINDIR=/usr/mdec
S= ${.CURDIR}/../../../..
LIBSA=${S}/lib/libsa
CPPFLAGS+= -I${LIBSA} -I. -I${.CURDIR}/../libsa
WARNS?= 1

# Need this to work in the miniroot
LDSTATIC?=	-static

.PATH.c: ${LIBSA} ${S}/ufs/ffs

SRCS=	installboot.c byteorder.c ffs_bswap.c \
	loadfile.c loadfile_aout.c loadfile_elf32.c

.include <bsd.prog.mk>
