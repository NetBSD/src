#	$NetBSD: Makefile,v 1.28 2003/10/19 05:30:30 lukem Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 4/19/94

PROG=	config
BINDIR=	/usr/sbin
SRCS=	files.c gram.y hash.c main.c mkdevsw.c mkheaders.c mkioconf.c \
	mkmakefile.c mkswap.c pack.c scan.l sem.c util.c
MAN=	config.8
YHEADER=1
CPPFLAGS+=-I${.CURDIR}

CWARNFLAGS+=-Wno-format-y2k

.include <bsd.prog.mk>
