#	$NetBSD: Makefile,v 1.3.2.1.2.1 2005/04/06 11:22:40 tron Exp $

.include <bsd.own.mk>

.include "../../Xserver/Makefile.common"

PROG=	xinit
MAN=	xinit.1 startx.1

CPPFLAGS+=	-DHAS_VFORK ${X11FLAGS.CONNECTION} -DBINDIR=\"${X11BINDIR}\" \

CPPFLAGS+=	-DXFREE86

CPPSCRIPTS=	startx xinitrc
CPPSCRIPTFLAGS_xinitrc=	-DXINITDIR=${X11LIBDIR}/xinit -DBINDIR=${X11BINDIR}
CPPSCRIPTFLAGS_startx=	-DXINITDIR=${X11LIBDIR}/xinit -DBINDIR=${X11BINDIR} \
		-DHAS_COOKIE_MAKER \
		-DMK_COOKIE='dd if=/dev/urandom bs=16 count=1 2>/dev/null | hexdump -e \\"%08x\\"'

SCRIPTS=	startx
FILES=		xinitrc
FILESDIR=	${X11LIBDIR}/xinit

LDADD+=	-lXmuu -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXMUU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

X11EXTRAMANDEFS=-D__xinitdir__=${X11LIBDIR}/xinit

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
