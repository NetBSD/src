#	$NetBSD: Makefile,v 1.4 2004/08/04 08:37:04 lukem Exp $

.include <bsd.own.mk>

LIB=	Xext
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	DPMS.c MITMisc.c XAppgroup.c XEVI.c XLbx.c XMultibuf.c XSecurity.c \
	XShape.c XShm.c XSync.c XTestExt1.c Xcup.c Xdbe.c extutil.c globals.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}
CPPFLAGS+=	-I${X11SRCDIR.xc}/lib/X11

LIBDPLIBS=X11 ../X11/dynamic

.include <bsd.x11.mk>
.include <bsd.lib.mk>
