#	$NetBSD: Makefile,v 1.4.8.1 2000/11/20 20:15:29 bouyer Exp $

.include <bsd.own.mk>

PROG=	installboot
MKMAN=	no
BINDIR=/usr/mdec
COPTS+= -Wall -Wstrict-prototypes -Wmissing-prototypes

# Need this to work in the miniroot
LDSTATIC?=	-static

.include <bsd.prog.mk>
