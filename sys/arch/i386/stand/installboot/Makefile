#	$NetBSD: Makefile,v 1.6 1998/07/28 20:10:53 drochner Exp $

.include <bsd.own.mk>

BINDIR=	/usr/mdec
PROG=	installboot

SRCS=	installboot.c bootblks.c getmount.c

NOMAN=	yes

CPPFLAGS= -I${.CURDIR}/../lib/crt/bootsect
CFLAGS+= -Wall -Wmissing-prototypes -Wstrict-prototypes
LDSTATIC?=-static

.include <bsd.prog.mk>
