#	$NetBSD: Makefile,v 1.2 1999/02/13 02:54:19 lukem Exp $

PROG=	netstat
SRCS=	if.c inet.c main.c mbuf.c route.c

SRCDIR= ${.CURDIR}/../../../usr.bin/netstat
CPPFLAGS+= -DSMALL -I${SRCDIR}
LDADD=	-lkvm
DPADD=	${LIBKVM}
MKMAN=	no
BINOWN=	root
BINMODE=4555

all: ${PROG}

.PATH:  ${SRCDIR}

.include <bsd.prog.mk>
