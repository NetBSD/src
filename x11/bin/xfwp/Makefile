#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:21 lukem Exp $

.include <bsd.own.mk>

PROG=	xfwp
SRCS=	xfwp.c misc.c pm.c transport.c io.c

CPPFLAGS.transport.c=	-DBSD44SOCKETS

LDADD+=	-lICE
DPADD+=	${LIBICE}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
