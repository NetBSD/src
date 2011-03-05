#	$NetBSD: Makefile,v 1.1.1.1.20.1 2011/03/05 15:09:09 bouyer Exp $

.include <bsd.own.mk>

PROG=	xkbevd
SRCS=	xkbevd.c cfgscan.c cfgparse.y evargs.c printev.c utils.c

CPPFLAGS+=-DDFLT_XKB_CONFIG_ROOT='"${X11ROOTDIR}/lib/X11/xkb"'
CPPFLAGS+=-DDFLT_SOUND_CMD='"/usr/bin/audioplay"'
CPPFLAGS+=-DDFLT_SOUND_DIR='"${X11ROOTDIR}/share/sounds/"'
CPPFLAGS+=-I${X11SRCDIR.${PROG}}
LDADD+=	-lxkbfile -lXext -lX11
DPADD+=	${LIBXKBFILE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
