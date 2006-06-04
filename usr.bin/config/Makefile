#	$NetBSD: Makefile,v 1.3 2006/06/04 19:42:19 cube Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 4/19/94

PROG=	config
SRCS=	files.c gram.y hash.c main.c mkdevsw.c mkheaders.c mkioconf.c \
	mkmakefile.c mkswap.c pack.c scan.l sem.c util.c
MAN=	config.1
MAN+=	config.5 config.samples.5
YHEADER=1
CPPFLAGS+=-I${.CURDIR} -I.

CWARNFLAGS+=-Wno-format-y2k

.include <bsd.prog.mk>
