#	$NetBSD: Makefile,v 1.8 2001/08/20 12:19:45 martin Exp $

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
