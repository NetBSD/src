#	$NetBSD: Makefile,v 1.2 1997/03/22 00:16:56 thorpej Exp $

BINDIR=	/usr/mdec
PROG=	installboot

MAN=	installboot.8
MANSUBDIR=/i386

CPPFLAGS= -I${.CURDIR}/../lib/crt/bootsect
CFLAGS+= -Wall
LDSTATIC=-static

.include <bsd.prog.mk>
