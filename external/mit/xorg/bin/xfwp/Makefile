#	$NetBSD: Makefile,v 1.1.1.1.6.2 2008/10/19 22:41:01 haad Exp $

.include <bsd.own.mk>

PROG=	xfwp
SRCS=	xfwp.c misc.c pm.c transport.c io.c

CPPFLAGS.transport.c=	-DBSD44SOCKETS

LDADD+=	-lICE
DPADD+=	${LIBICE}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
