#	$NetBSD: Makefile,v 1.11 1999/02/14 02:56:19 dean Exp $

# XXX: bsd.own.mk included so LDSTATIC can be overwridden
.include <bsd.own.mk>

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
