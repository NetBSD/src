#	$NetBSD: Makefile,v 1.5 1998/03/02 19:57:01 cgd Exp $

.include <bsd.own.mk>

BINDIR=	/usr/mdec
PROG=	installboot

SRCS=	installboot.c bootblks.c getmount.c

NOMAN=	yes

CPPFLAGS= -I${.CURDIR}/../lib/crt/bootsect
CFLAGS+= -Wall
LDSTATIC?=-static

.include <bsd.prog.mk>
