#	$NetBSD: Makefile,v 1.12 1999/03/14 05:50:39 sommerfe Exp $

# XXX: bsd.own.mk included so LDSTATIC can be overwridden
.include <bsd.own.mk>

CFLAGS=-g
BINDIR=	/usr/mdec
PROG=	installboot
.PATH.c: ${.CURDIR}/../lib

SRCS=	installboot.c bootblks.c getmount.c loadfile.c

MKMAN=	no

CPPFLAGS+= -I${.CURDIR}/../lib/crt/bootsect
CPPFLAGS+= -I${.CURDIR}/../lib
CFLAGS+= -Wall -Wmissing-prototypes -Wstrict-prototypes
LDSTATIC?=-static

.include <bsd.prog.mk>
