#	$NetBSD: Makefile,v 1.6.2.1 2004/11/15 09:12:46 rtr Exp $

.include <bsd.own.mk>

LIB=	Xrender

XRENDERDIR=	${X11SRCDIR.xc}/lib/Xrender
.PATH:		${XRENDERDIR}
SRCS=		Xrender.c AddTrap.c Color.c Composite.c Cursor.c \
		Glyph.c Picture.c FillRect.c FillRects.c Filter.c \
		Poly.c Trap.c Tri.c

INCS=	Xrender.h
INCSDIR=${X11INCDIR}/X11/extensions

FILES=		xrender.pc
FILESDIR_xrender.pc=	${X11USRLIBDIR}/pkgconfig

CLEANFILES+=	${FILES}
realall:	${FILES}

XCURSORDIR=	${X11SRCDIR.xc}/lib/Xcursor
CONFIG_SUBST= 	${HOST_SH} ${XCURSORDIR}/config-subst \
		prefix="${X11ROOTDIR}" \
		exec_prefix="${X11BINDIR}" \
		libdir="${X11USRLIBDIR}" \
		includedir="${X11INCDIR}" \
		VERSION="0.8.4" \
		X_REQUIRES="" \
		RENDER_CFLAGS="" \
		X_NON_PKG_CFLAGS="" \
		X_NON_PKG_LIBS="-lX11 -lXext"

xrender.pc:
	${CONFIG_SUBST} < ${XRENDERDIR}/xrender.pc.in > ${.TARGET}

CPPFLAGS+=	${X11FLAGS.THREADLIB}
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/extensions
CPPFLAGS+=	-I${X11SRCDIR.xc}/lib/X11

LIBDPLIBS=\
	Xext	${.CURDIR}/../Xext \
	X11	${.CURDIR}/../X11/dynamic

.include <bsd.x11.mk>
.include <bsd.lib.mk>
