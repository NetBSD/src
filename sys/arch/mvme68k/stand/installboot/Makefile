#	$NetBSD: Makefile,v 1.16.122.2 2017/12/03 11:36:30 jdolecek Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	installboot
BINDIR=/usr/mdec
LIBSA=${.CURDIR}/../../../../lib/libsa
COPTS+= -Wall -Wstrict-prototypes -Wmissing-prototypes
CPPFLAGS+= -I${LIBSA} -I. -D__daddr_t=int32_t -D_KERNTYPES
# Need this to work in the miniroot
LDSTATIC= -static

.PATH.c: ${LIBSA}

SRCS=	installboot.c byteorder.c loadfile.c loadfile_aout.c loadfile_elf32.c

.include <bsd.prog.mk>
.include <bsd.klinks.mk>
