#	$NetBSD: Makefile,v 1.16 1997/10/18 04:08:08 lukem Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 4/19/94

WARNS?=	0
PROG=	config
BINDIR=	/usr/sbin
SRCS=	files.c gram.y hash.c main.c mkheaders.c mkioconf.c mkmakefile.c \
	mkswap.c pack.c scan.l sem.c util.c
CFLAGS+=-I${.CURDIR} -I.

# This program actually requires "flex" (not just any old lex).
# Also note that it does not use -ll
LEX=flex

CLEANFILES+= y.tab.h

MAN=	config.8

.include <bsd.prog.mk>
