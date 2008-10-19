#	$NetBSD: Makefile,v 1.1.1.1.6.2 2008/10/19 22:41:02 haad Exp $

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
