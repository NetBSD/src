#	$NetBSD: Makefile,v 1.3 1999/05/19 04:17:50 gwr Exp $

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

.PATH:  ${SRCDIR}

.include <bsd.prog.mk>
