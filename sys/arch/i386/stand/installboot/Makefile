#	$NetBSD: Makefile,v 1.13 1999/04/28 09:14:22 christos Exp $

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
CPPFLAGS+= -I${LIBSA}
CFLAGS+= -Wall -Wmissing-prototypes -Wstrict-prototypes
LDSTATIC?=-static

.include <bsd.prog.mk>
