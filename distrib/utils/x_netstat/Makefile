#	$NetBSD: Makefile,v 1.10 2001/12/12 00:05:13 tv Exp $

SRCDIR=		${.CURDIR}/../../../usr.bin/netstat

PROG=		netstat
NOMAN=		# defined

SRCS=		if.c inet.c main.c mbuf.c route.c

CPPFLAGS+=	-DSMALL -I${SRCDIR}
DPADD=		${LIBKVM}
LDADD=		-lkvm

.include <bsd.prog.mk>

BINOWN=		root
BINMODE=	4555
.PATH:		${SRCDIR}
