#	$NetBSD: Makefile,v 1.1 1999/01/10 13:50:35 mrg Exp $

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
