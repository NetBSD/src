#	$NetBSD: Makefile,v 1.2.4.2 2017/04/26 02:52:18 pgoyette Exp $

.include <bsd.own.mk>

WARNS=5

PROG=	bc
BINDIR= /usr/bin

SRCDIR= ${.CURDIR}/../dist
.PATH:  ${SRCDIR}

SRCS=	 	global.c load.c util.c execute.c main.c number.c storage.c \
		bc.y scan.l warranty.c

CPPFLAGS+=	-I. -I${SRCDIR} -D_POSIX_SOURCE
CFLAGS+=	-funsigned-char
LFLAGS+=	-I8
YHEADER=	1

LDADD+= -ll -ledit -ltermcap
DPADD+=	${LIBL} ${LIBEDIT} ${LIBTERMCAP}

.include <bsd.prog.mk>
