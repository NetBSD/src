#	$NetBSD: Makefile,v 1.2 2005/06/06 06:12:09 thorpej Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 4/19/94

PROG=	config
SRCS=	files.c gram.y hash.c main.c mkdevsw.c mkheaders.c mkioconf.c \
	mkmakefile.c mkswap.c pack.c scan.l sem.c util.c
MAN=	config.1
YHEADER=1
CPPFLAGS+=-I${.CURDIR} -I.

CWARNFLAGS+=-Wno-format-y2k

.include <bsd.prog.mk>
