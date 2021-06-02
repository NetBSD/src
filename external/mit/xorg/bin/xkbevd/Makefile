#	$NetBSD: Makefile,v 1.3.24.1 2021/06/02 14:46:21 cjep Exp $

.include <bsd.own.mk>

PROG=	xkbevd
SRCS=	xkbevd.c cfgscan.c cfgparse.y evargs.c printev.c utils.c

CPPFLAGS+=-DDFLT_XKB_CONFIG_ROOT='"${X11ROOTDIR}/lib/X11/xkb"'
CPPFLAGS+=-DDFLT_SOUND_CMD='"/usr/bin/audioplay"'
CPPFLAGS+=-DDFLT_SOUND_DIR='"${X11ROOTDIR}/share/sounds/"'
CPPFLAGS+=-DHAVE_STRCASECMP
CPPFLAGS+=-I${X11SRCDIR.${PROG}}
LDADD+=	${XLIBLDADD_KDFILE}
DPADD+=	${XLIBDPADD_KDFILE}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
