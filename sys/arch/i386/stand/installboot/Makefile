#	$NetBSD: Makefile,v 1.4 1997/06/22 06:01:37 mrg Exp $

BINDIR=	/usr/mdec
PROG=	installboot

SRCS=	installboot.c bootblks.c getmount.c

NOMAN=	yes

CPPFLAGS= -I${.CURDIR}/../lib/crt/bootsect
CFLAGS+= -Wall
LDSTATIC=-static

.include <bsd.prog.mk>
