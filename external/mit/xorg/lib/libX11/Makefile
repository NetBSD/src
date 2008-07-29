#	$NetBSD: Makefile,v 1.1.1.1 2008/07/29 05:01:21 mrg Exp $

.include <bsd.own.mk>

SUBDIR=		dynamic static

INCS=		ImUtil.h XKBlib.h Xcms.h Xlib.h XlibConf.h Xlibint.h \
		Xlocale.h Xregion.h Xresource.h Xutil.h cursorfont.h \
		Xcmsint.h \
		Xlcint.h
INCSDIR=	${X11INCDIR}/X11

FILES=		XErrorDB XKeysymDB Xcms.txt
FILESDIR=	${X11LIBDIR}

.PATH:		${X11SRCDIR.X11}/include/X11
.PATH:		${X11SRCDIR.X11}/src
.PATH:		${X11SRCDIR.X11}/src/xcms
.PATH:		${X11SRCDIR.X11}/src/xlibi18n

XlibConf.h: XlibConf.h.in
	${TOOL_SED} 's/#undef XTHREADS/#define XTHREADS 1/; \
		    s/#undef XUSE_MTSAFE_API/#define XUSE_MTSAFE_API/ ' \
		< ${.ALLSRC} > ${.TARGET}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
