#	$NetBSD: Makefile,v 1.4 1999/05/23 10:25:43 veego Exp $

PROG=	netstat
SRCS=	if.c inet.c main.c mbuf.c route.c

SRCDIR= ${BSDSRCDIR}/usr.bin/netstat
CPPFLAGS+= -DSMALL -I${SRCDIR}
LDADD=	-lkvm
DPADD=	${LIBKVM}
MKMAN=	no
BINOWN=	root
BINMODE=4555

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
