#	$NetBSD: Makefile,v 1.9 1999/02/12 05:15:35 cjs Exp $

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
