#	$NetBSD: Makefile,v 1.24.6.1 2002/05/16 13:09:19 gehenna Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 4/19/94

PROG=	config
BINDIR=	/usr/sbin
SRCS=	files.c gram.y hash.c main.c mkdevsw.c mkheaders.c mkioconf.c \
	mkmakefile.c mkswap.c pack.c scan.l sem.c util.c
YFLAGS=
CPPFLAGS+=-I${.CURDIR} -I.
CLEANFILES+=gram.h

MAN=	config.8

# XXX should use "YHEADER", but make this work on 1.3 as well as 1.4+
gram.h: gram.c
gram.c: gram.y
	${YACC.y} -d ${.ALLSRC}
	mv -f y.tab.c gram.c
	mv -f y.tab.h gram.h

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
