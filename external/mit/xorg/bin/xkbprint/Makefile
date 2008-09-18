#	$NetBSD: Makefile,v 1.1.1.1.2.2 2008/09/18 05:18:30 wrstuden Exp $

.include <bsd.own.mk>

PROG=	xkbprint
SRCS=	xkbprint.c utils.c psgeom.c

LDADD+=	-lxkbfile -lXext -lX11
DPADD+=	${LIBXKBFILE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}

# XXX
CPPFLAGS.psgeom.c+=	-Wno-error

.include <bsd.x11.mk>
.include <bsd.prog.mk>
