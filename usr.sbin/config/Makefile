#	$NetBSD: Makefile,v 1.21 1998/05/01 13:02:09 tv Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 4/19/94

PROG=	config
BINDIR=	/usr/sbin
SRCS=	files.c gram.y hash.c main.c mkheaders.c mkioconf.c mkmakefile.c \
	mkswap.c pack.c scan.l sem.c util.c
YFLAGS=
CPPFLAGS+=-I${.CURDIR} -I.
CLEANFILES+=gram.h

# This program actually requires "flex" (not just any old lex).
# Also note that it does not use -ll
LEX=flex

MAN=	config.8

# XXX should use "YHEADER", but make this work on 1.3 as well as 1.4+
gram.c gram.h: gram.y
	${YACC.y} -d ${.ALLSRC}
	mv -f y.tab.c gram.c
	mv -f y.tab.h gram.h

.include <bsd.prog.mk>
