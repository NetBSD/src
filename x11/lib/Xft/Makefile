#	$NetBSD: Makefile,v 1.4 2004/01/06 06:15:45 rtr Exp $

.include <bsd.own.mk>

LIB=	Xft
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	xftcolor.c xftcore.c xftdbg.c xftdpy.c xftdraw.c xftextent.c \
	xftfont.c xftfreetype.c xftglyphs.c xftinit.c xftlist.c \
	xftname.c xftrender.c xftstr.c xftswap.c xftxlfd.c

INCS=	Xft.h XftCompat.h
INCSDIR=${X11INCDIR}/X11/Xft

MAN=	Xft.3

CPPFLAGS+=	${X11FLAGS.THREADLIB}
CPPFLAGS+=	-DFREETYPE2 -I${DESTDIR}${X11INCDIR}/freetype2

# XXX it would be really swell if SCRIPTS was available from bsd.lib.mk
FILES=			xft.pc xft-config
FILESDIR_xft.pc=	${X11USRLIBDIR}/pkgconfig
FILESDIR_xft-config=	${X11BINDIR}
FILESOWN_xft-config=	${BINOWN}
FILESGRP_xft-config=	${BINGRP}
FILESMODE_xft-config=	${BINMODE}

.PHONY:		xft.pc xft-config
all dependall:	xft.pc xft-config

# XXX this should be ${SHELL} but I'm not getting joy w/ it.
XFTDIR=		${X11SRCDIR.xc}/lib/Xft
CONFIG_SUBST=	sh ${XFTDIR}/config/config-subst \
		prefix="${X11ROOTDIR}" \
		exec_prefix="${X11BINDIR}" \
		libdir="${X11USRLIBDIR}" \
		includedir="${X11INCDIR}" \
		PACKAGE_VERSION="2.1.0" \
		FREETYPE_LIBS="-L${X11USRLIBDIR} -lfreetype" \
		FREETYPE_CFLAGS="-I${X11INCDIR}/freetype2" \
		XRENDER_LIBS="-L${X11USRLIBDIR} -lXrender" \
		XRENDER_CFLAGS="-I${X11INCDIR}" \
		FONTCONFIG_CFLAGS="-I${X11INCDIR}" 

xft.pc:
	${CONFIG_SUBST} < ${XFTDIR}/xft.pc.in > ${.TARGET}

xft-config:
	${CONFIG_SUBST} < ${XFTDIR}/xft-config.in > ${.TARGET}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
