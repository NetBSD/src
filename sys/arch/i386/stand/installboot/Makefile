#	$NetBSD: Makefile,v 1.13.4.1 1999/11/15 00:38:13 fvdl Exp $

# XXX: bsd.own.mk included so LDSTATIC can be overwridden
.include <bsd.own.mk>

CFLAGS=-g
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

.BEGIN: ${.OBJDIR}/machine
depend all: ${.OBJDIR}/machine
CLEANFILES+= ${.OBJDIR}/machine

${.OBJDIR}/machine::
	-rm -f $@
	ln -s ${.CURDIR}/../../include $@

.include <bsd.prog.mk>
