#	$NetBSD: Makefile,v 1.9 2001/10/05 01:17:31 jmc Exp $

PROG=	netstat
SRCS=	if.c inet.c main.c mbuf.c route.c

SRCDIR= ${.CURDIR}/../../../usr.bin/netstat
CPPFLAGS+= -DSMALL -I${SRCDIR}
.if ${MACHINE_ARCH} != "sparc64"
DBG= -Os
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
