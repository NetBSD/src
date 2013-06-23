#	$NetBSD: Makefile,v 1.2.12.1 2013/06/23 06:28:42 tls Exp $

.include <bsd.own.mk>

PROG=	xmodmap
SRCS=	xmodmap.c pf.c handle.c exec.c

CPPFLAGS+=	-I${X11SRCDIR.${PROG}}/../include

LDADD+=	-lX11
DPADD+=	${LIBX11}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/man

COPTS.handle.c+=-Wno-error	# uses XKeycodeToKeysym
COPTS.exec.c+=	-Wno-error	# uses XKeycodeToKeysym

.include <bsd.x11.mk>
.include <bsd.prog.mk>
