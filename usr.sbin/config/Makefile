#	$NetBSD: Makefile,v 1.13 1997/05/09 07:50:34 mycroft Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 4/19/94

PROG=	config
BINDIR=	/usr/sbin
SRCS=	files.c gram.c hash.c main.c mkheaders.c mkioconf.c mkmakefile.c \
	mkswap.c pack.c scan.c sem.c util.c
CFLAGS+=-I${.CURDIR} -I.

# This program actually requires "flex" (not just any old lex).
# Also note that it does not use -ll
LEX=flex

CLEANFILES+=gram.c scan.c y.tab.h
MAN=	config.8

.include <bsd.prog.mk>
