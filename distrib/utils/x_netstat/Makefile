#	$NetBSD: Makefile,v 1.7.4.1 2001/07/29 20:32:32 he Exp $

PROG=	netstat
SRCS=	if.c inet.c main.c mbuf.c route.c

SRCDIR= ${.CURDIR}/../../../usr.bin/netstat
CPPFLAGS+= -DSMALL -I${SRCDIR}
.if ${MACHINE_ARCH} != "sparc64"
COPTS+= -Os
.endif
LDADD=	-lkvm
LDSTATIC?=-static
DPADD=	${LIBKVM}
MKMAN=	no
BINOWN=	root
BINMODE=4555

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
