# $NetBSD: Makefile,v 1.1 1999/07/02 17:41:24 itojun Exp $

PROG=	setkey
SRCS=	setkey.c parse.y token.l
LDADD+=	-ll -ly
CLEANFILES+=	y.tab.c y.tab.h key_test.o keytest
YFLAGS+=

CPPFLAGS+=-DIPSEC_DEBUG -DYY_NO_UNPUT -DINET6
CPPFLAGS+=-I.

#SCRIPTS=	scriptdump

BINOWN  = root
BINGRP  = bin
BINMODE = 555

all: ${PROG} ${SCRIPTS}

# FreeBSD
#SRCS+=y.tab.h
#y.tab.h: parse.y

LDADD+= -lipsec
DPADD+=	${LIBIPSEC}
#CLEANFILES+=	scriptdump

MAN=	setkey.8
LOCALPREFIX=	/usr/pkg

#scriptdump: scriptdump.pl
#	sed -e 's#@LOCALPREFIX@#${LOCALPREFIX}#' < ${.CURDIR}/scriptdump.pl > scriptdump

parse.c parse.h: parse.y
	${YACC.y} -d ${.ALLSRC}
	mv -f y.tab.c parse.c
	mv -f y.tab.h parse.h

.include <bsd.prog.mk>
