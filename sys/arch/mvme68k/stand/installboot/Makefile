#	$NetBSD: Makefile,v 1.9.2.2 2002/01/08 00:26:37 nathanw Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	installboot
BINDIR=/usr/mdec
LIBSA=${.CURDIR}/../../../../lib/libsa
COPTS+= -Wall -Wstrict-prototypes -Wmissing-prototypes
CPPFLAGS+= -I${LIBSA} -I.
# Need this to work in the miniroot
LDSTATIC?=	-static

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
