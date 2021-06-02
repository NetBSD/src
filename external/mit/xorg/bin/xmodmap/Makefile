#	$NetBSD: Makefile,v 1.5.24.1 2021/06/02 14:46:22 cjep Exp $

.include <bsd.own.mk>

PROG=	xmodmap
SRCS=	xmodmap.c pf.c handle.c exec.c

CPPFLAGS+=	-DHAVE_ASPRINTF -DHAVE_STRNCASECMP

LDADD+=	${XLIBLDADD_X11}
DPADD+=	${XLIBDPADD_X11}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/man

COPTS.handle.c+=-Wno-error	# uses XKeycodeToKeysym
COPTS.exec.c+=	-Wno-error	# uses XKeycodeToKeysym

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
