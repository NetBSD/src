#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:29 lukem Exp $

.include <bsd.own.mk>

PROG=	xkbevd
SRCS=	xkbevd.c cfgscan.c cfgparse.y evargs.c printev.c utils.c

LDADD+=	-lxkbfile -lXext -lX11
DPADD+=	${LIBXKBFILE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
