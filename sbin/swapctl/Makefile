#	$NetBSD: Makefile,v 1.5 2006/08/27 21:07:39 martin Exp $

PROG=	swapctl
SRCS=	swapctl.c swaplist.c
MAN=	swapctl.8
LDADD+=	-lutil
DPADD+=	${LIBUTIL}

LINKS=	${BINDIR}/swapctl ${BINDIR}/swapon
MLINKS=	swapctl.8 swapon.8

.include <bsd.prog.mk>
