#	$NetBSD: Makefile,v 1.6 2000/05/18 00:43:07 matt Exp $

PROG=	netstat
SRCS=	if.c inet.c main.c mbuf.c route.c

SRCDIR= ${.CURDIR}/../../../usr.bin/netstat
CPPFLAGS+= -DSMALL -I${SRCDIR}
COPTS+= -Os
LDADD=	-lkvm
DPADD=	${LIBKVM}
MKMAN=	no
BINOWN=	root
BINMODE=4555

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
