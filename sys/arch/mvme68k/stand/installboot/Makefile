#	$NetBSD: Makefile,v 1.16 2005/12/28 08:08:00 skrll Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	installboot
BINDIR=/usr/mdec
LIBSA=${.CURDIR}/../../../../lib/libsa
COPTS+= -Wall -Wstrict-prototypes -Wmissing-prototypes
CPPFLAGS+= -I${LIBSA} -I. -D__daddr_t=int32_t
# Need this to work in the miniroot
LDSTATIC= -static

.if !make(obj) && !make(clean) && !make(cleandir)
.BEGIN: machine
.NOPATH: machine

machine::
	-rm -f $@
	ln -s ${.CURDIR}/../../include $@
.endif

CLEANFILES+= machine

.PATH.c: ${LIBSA}

SRCS=	installboot.c byteorder.c loadfile.c loadfile_aout.c loadfile_elf32.c

.include <bsd.prog.mk>
