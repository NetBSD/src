#	$NetBSD: Makefile,v 1.13 2003/02/04 13:34:56 matt Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	installboot
BINDIR=/usr/mdec
LIBSA=${.CURDIR}/../../../../lib/libsa
COPTS+= -Wall -Wstrict-prototypes -Wmissing-prototypes
CPPFLAGS+= -I${LIBSA} -I. -D__daddr_t=int32_t
# Need this to work in the miniroot
LDSTATIC= -static

.BEGIN: machine
.NOPATH: machine
realdepend realall: machine
CLEANFILES+= machine

.PATH.c: ${LIBSA}

SRCS=	installboot.c byteorder.c loadfile.c loadfile_aout.c loadfile_elf32.c

machine::
	-rm -f $@
	ln -s ${.CURDIR}/../../include $@

.include <bsd.prog.mk>
