#	$NetBSD: Makefile,v 1.8 1999/01/29 18:41:50 christos Exp $

.include <bsd.own.mk>

BINDIR=	/usr/mdec
PROG=	installboot
.PATH.c: ${.CURDIR}/../lib

SRCS=	installboot.c bootblks.c getmount.c loadfile.c

NOMAN=	yes

CPPFLAGS+= -I${.CURDIR}/../lib/crt/bootsect
CPPFLAGS+= -I${.CURDIR}/../lib
CFLAGS+= -Wall -Wmissing-prototypes -Wstrict-prototypes
LDSTATIC?=-static

.include <bsd.prog.mk>
