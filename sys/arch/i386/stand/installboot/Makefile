#	$NetBSD: Makefile,v 1.10 1999/02/12 23:48:46 dean Exp $

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
