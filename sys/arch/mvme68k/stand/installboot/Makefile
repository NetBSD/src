#	$NetBSD: Makefile,v 1.6.4.1 2001/08/25 06:15:38 thorpej Exp $

.include <bsd.own.mk>

PROG=	installboot
MKMAN=	no
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

SRCS=	installboot.c loadfile.c

machine::
	-rm -f $@
	ln -s ${.CURDIR}/../../include $@

.include <bsd.prog.mk>
