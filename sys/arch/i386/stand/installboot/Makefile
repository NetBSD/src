#	$NetBSD: Makefile,v 1.14 1999/10/25 20:27:48 drochner Exp $

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
