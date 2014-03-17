#	$NetBSD: Makefile,v 1.4 2014/03/17 10:11:27 mrg Exp $

.include <bsd.own.mk>

PROG=	xmodmap
SRCS=	xmodmap.c pf.c handle.c exec.c

CPPFLAGS+=	-I${X11SRCDIR.${PROG}}/../include -DHAVE_CONFIG_H

LDADD+=	-lX11
DPADD+=	${LIBX11}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/man

COPTS.handle.c+=-Wno-error	# uses XKeycodeToKeysym
COPTS.exec.c+=	-Wno-error	# uses XKeycodeToKeysym

.include <bsd.x11.mk>
.include <bsd.prog.mk>
