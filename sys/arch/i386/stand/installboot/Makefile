#	$NetBSD: Makefile,v 1.16.12.1 2001/10/11 00:01:45 fvdl Exp $

# XXX: bsd.own.mk included so LDSTATIC can be overwridden
.include <bsd.own.mk>

BINDIR=	/usr/mdec
PROG=	installboot
LIBSA=${.CURDIR}/../../../../lib/libsa

.PATH.c: ${LIBSA}

SRCS=	installboot.c bootblks.c getmount.c loadfile.c

MKMAN=	no

CPPFLAGS+= -I${.CURDIR}/../lib/crt/bootsect
CPPFLAGS+= -I${LIBSA} -I.
CFLAGS+= -Wall -Wmissing-prototypes -Wstrict-prototypes
LDSTATIC?=-static

.BEGIN: machine
.NOPATH: machine
${SRCS} realdepend realall: machine
CLEANFILES+= machine

machine::
	-rm -f $@
	ln -s ${.CURDIR}/../../include $@

.include <bsd.prog.mk>
