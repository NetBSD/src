#	$NetBSD: Makefile,v 1.7 1999/01/28 22:47:25 christos Exp $

.include <bsd.own.mk>

BINDIR=	/usr/mdec
PROG=	installboot
.PATH.c: ${.CURDIR}/../lib

SRCS=	installboot.c bootblks.c getmount.c loadfile.c

NOMAN=	yes

CPPFLAGS+= -I${.CURDIR}/../lib/crt/bootsect
CPPFLAGS+= -DINSTALLBOOT -I${.CURDIR}/../lib
CFLAGS+= -Wall -Wmissing-prototypes -Wstrict-prototypes -g
LDSTATIC?=-static

.include <bsd.prog.mk>
