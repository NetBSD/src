#	$NetBSD: Makefile,v 1.5 2000/07/24 18:39:37 jdolecek Exp $

.include <bsd.own.mk>

PROG=	installboot
MKMAN=	no
BINDIR=/usr/mdec
COPTS+= -Wall -Wstrict-prototypes -Wmissing-prototypes

# Need this to work in the miniroot
LDSTATIC?=	-static

.include <bsd.prog.mk>
