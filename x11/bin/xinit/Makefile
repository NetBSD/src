#	$NetBSD: Makefile,v 1.3 2003/09/13 21:50:52 lukem Exp $

.include <bsd.own.mk>

PROG=	xinit
MAN=	xinit.1 startx.1

CPPFLAGS+=-DHAS_VFORK ${X11FLAGS.CONNECTION} -DBINDIR=\"${X11BINDIR}\"

CPPSCRIPTS=	startx xinitrc
CPPSCRIPTFLAGS=	-DXINITDIR=${X11LIBDIR}/xinit -DBINDIR=${X11BINDIR}
SCRIPTS=	startx
FILES=		xinitrc
FILESDIR=	${X11LIBDIR}/xinit

LDADD+=	-lXmuu -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXMUU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

X11EXTRAMANDEFS=-D__xinitdir__=${X11LIBDIR}/xinit

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
