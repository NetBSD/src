#	$NetBSD: Makefile,v 1.1.1.1.6.2 2008/10/19 22:41:03 haad Exp $

.include <bsd.own.mk>

PROG=	xmodmap
SRCS=	xmodmap.c pf.c handle.c exec.c

CPPFLAGS+=	-I${X11SRCDIR.${PROG}}/../include

LDADD+=	-lX11
DPADD+=	${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
