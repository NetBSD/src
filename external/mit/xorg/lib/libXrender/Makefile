#	$NetBSD: Makefile,v 1.1.1.1 2008/07/29 05:01:22 mrg Exp $

.include <bsd.own.mk>

LIB=	Xrender

XRENDERDIR=	${X11SRCDIR.${LIB}}
.PATH:		${XRENDERDIR}/src
.PATH:		${XRENDERDIR}/include/X11/extensions
SRCS=		Xrender.c AddTrap.c Color.c Composite.c Cursor.c \
		Glyph.c Picture.c FillRect.c FillRects.c Filter.c \
		Poly.c Trap.c Tri.c

INCS=	Xrender.h
INCSDIR=${X11INCDIR}/X11/extensions

.if 0
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
		VERSION="0.9.4" \
		X_REQUIRES="" \
		RENDER_CFLAGS="" \
		X_NON_PKG_CFLAGS="" \
		X_NON_PKG_LIBS="-lX11 -lXext"

xrender.pc:
	${CONFIG_SUBST} < ${XRENDERDIR}/xrender.pc.in > ${.TARGET}
.endif

CPPFLAGS+=	${X11FLAGS.THREADLIB}
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/extensions
CPPFLAGS+=	-I${XRENDERDIR}/../include
#CPPFLAGS+=	-I${X11SRCDIR.xc}/lib/X11

LIBDPLIBS=\
	Xext	${.CURDIR}/../libXext \
	X11	${.CURDIR}/../libX11/dynamic

.include <bsd.x11.mk>
.include <bsd.lib.mk>
