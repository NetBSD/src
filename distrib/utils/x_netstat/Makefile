#	$NetBSD: Makefile,v 1.1.2.2 1999/01/19 07:34:50 cgd Exp $

PROG=	netstat
SRCS=	if.c inet.c main.c mbuf.c route.c

SRCDIR= ${.CURDIR}/../../../usr.bin/netstat
CPPFLAGS+= -DSMALL -I${SRCDIR}
LDADD=	-lkvm
DPADD=	${LIBKVM}
NOMAN=
BINOWN=	root
BINMODE=4555

all: ${PROG}

.PATH:  ${SRCDIR}

.include <bsd.prog.mk>
