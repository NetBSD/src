#	$NetBSD: Makefile,v 1.19 1999/01/16 05:57:47 simonb Exp $

TOP=	${.CURDIR}/../..
dist=	${TOP}/dist
srcdir=	${dist}/bc

PROG=	bc
CPPFLAGS+=-D_POSIX_SOURCE -I${.CURDIR} -I${srcdir}/bc -I${srcdir}/h
LFLAGS+=-I8
SRCS=	bc.y execute.c getopt.c getopt1.c global.c load.c main.c number.c \
	scan.l storage.c util.c
YHEADER=1
LDADD+=	-ll -ledit -ltermcap
DPADD+=	${LIBL} ${LIBEDIT} ${LIBTERMCAP}

.PATH:	${srcdir}/bc
.PATH:	${srcdir}/lib
.PATH:	${srcdir}/doc

.include <bsd.prog.mk>
