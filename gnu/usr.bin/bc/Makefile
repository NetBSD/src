#	$NetBSD: Makefile,v 1.20 1999/01/18 04:12:15 simonb Exp $

TOP=	${.CURDIR}/../..
dist=	${TOP}/dist
srcdir=	${dist}/bc

PROG=	bc
CPPFLAGS+=-D_POSIX_SOURCE -I. -I${.CURDIR} -I${srcdir}/bc -I${srcdir}/h
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
