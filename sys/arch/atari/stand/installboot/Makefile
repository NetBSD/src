#	$NetBSD: Makefile,v 1.4.22.1 2002/01/10 19:40:16 thorpej Exp $

PROG=	installboot
NOMAN=	# defined
SRCS=	installboot.c disklabel.c
BINDIR=	/usr/mdec
LDADD=	-lkvm

CFLAGS+=-Wall

.include <bsd.prog.mk>
