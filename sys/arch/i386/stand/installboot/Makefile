#	$NetBSD: Makefile,v 1.3 1997/06/13 22:14:58 drochner Exp $

BINDIR=	/usr/mdec
PROG=	installboot

SRCS=	installboot.c bootblks.c getmount.c

MAN=	installboot.8
MANSUBDIR=/i386

CPPFLAGS= -I${.CURDIR}/../lib/crt/bootsect
CFLAGS+= -Wall
LDSTATIC=-static

.include <bsd.prog.mk>
