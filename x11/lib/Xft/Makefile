#	$NetBSD: Makefile,v 1.7.2.1 2004/04/29 06:08:31 jmc Exp $

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
CPPFLAGS+=	-DFREETYPE2 -I${DESTDIR}${X11INCDIR}/freetype2 -DXFREE86_FT2

# XXX it would be really swell if SCRIPTS was available from bsd.lib.mk
FILES=			xft.pc xft-config
FILESDIR_xft.pc=	${X11USRLIBDIR}/pkgconfig
FILESDIR_xft-config=	${X11BINDIR}
FILESOWN_xft-config=	${BINOWN}
FILESGRP_xft-config=	${BINGRP}
FILESMODE_xft-config=	${BINMODE}

CLEANFILES+=${FILES}
realall: ${FILES}

XFTDIR=		${X11SRCDIR.xc}/lib/Xft
CONFIG_SUBST=	${HOST_SH} ${XFTDIR}/config/config-subst \
		prefix="${X11ROOTDIR}" \
		exec_prefix="${X11BINDIR}" \
		libdir="${X11USRLIBDIR}" \
		includedir="${X11INCDIR}" \
		hardcode_libdir_flag_spec="-Wl,-rpath,${X11USRLIBDIR}" \
		PACKAGE_VERSION="2.1.0" \
		FREETYPE_LIBS="-L${X11USRLIBDIR} -lfreetype" \
		FREETYPE_CFLAGS="-I${X11INCDIR}/freetype2" \
		XRENDER_LIBS="-L${X11USRLIBDIR} -lXrender" \
		XRENDER_CFLAGS="-I${X11INCDIR}" \
		FONTCONFIG_LIBS="-L${X11USRLIBDIR} -lfontconfig" \
		FONTCONFIG_CFLAGS="-I${X11INCDIR}" 

xft.pc:
	${CONFIG_SUBST} < ${XFTDIR}/xft.pc.in > ${.TARGET}

xft-config:
	${CONFIG_SUBST} < ${XFTDIR}/xft-config.in > ${.TARGET}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
