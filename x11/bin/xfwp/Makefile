#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:07 lukem Exp $

.include <bsd.own.mk>

PROG=	xfwp
SRCS=	xfwp.c misc.c pm.c transport.c io.c

CPPFLAGS.transport.c=	-DBSD44SOCKETS

LDADD+=	-lICE
DPADD+=	${LIBICE}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.prog.mk>
.include <bsd.x11.mk>
