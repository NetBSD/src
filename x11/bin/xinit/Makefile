#	$NetBSD: Makefile,v 1.1.1.1 2003/09/12 02:52:15 lukem Exp $

.include <bsd.own.mk>

PROG=	xinit
MAN=	xinit.1 startx.1

CPPFLAGS+=-DHAS_VFORK ${X11FLAGS.CONNECTION} -DBINDIR=\"${X11BINDIR}\"

CPPSCRIPTS=	startx xinitrc
CPPSCRIPTFLAGS=	-DXINITDIR=${X11LIBDIR}/xinit -DBINDIR=${X11BINDIR}
SCRIPTS=	startx
FILES=		xinitrc
FILESDIR=	${X11LIBDIR}/xinit

LDADD+=	-lXmuu -lXext -lX11
DPADD+=	${LIBXMUU} ${LIBXEXT} ${LIBX11}

X11EXTRAMANDEFS=-D__xinitdir__=${X11LIBDIR}/xinit

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.prog.mk>
.include <bsd.x11.mk>
