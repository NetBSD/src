#	$NetBSD: Makefile,v 1.16.8.7 2002/12/11 06:01:05 thorpej Exp $

NOMAN=	# defined

# XXX: bsd.own.mk included so LDSTATIC can be overwridden
.include <bsd.own.mk>

PROG=	installboot
LIBSA=${.CURDIR}/../../../../lib/libsa

.PATH.c: ${LIBSA}

SRCS=	installboot.c bootblks.c getmount.c byteorder.c loadfile.c \
	loadfile_aout.c loadfile_elf32.c

.if ${MACHINE_ARCH} == "x86_64"
SRCS+=	loadfile_elf64.c
.endif

CPPFLAGS+= -I${.CURDIR}/../lib/crt/bootsect
CPPFLAGS+= -I${LIBSA} -I.
CFLAGS+= -Wall -Wmissing-prototypes -Wstrict-prototypes
LDSTATIC=-static

.BEGIN: machine
.NOPATH: machine
${SRCS} realdepend realall: machine
CLEANFILES+= machine

machine::
	-rm -f $@
	ln -s ${.CURDIR}/../../include $@

.include <bsd.prog.mk>
