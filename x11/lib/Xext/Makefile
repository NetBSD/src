#	$NetBSD: Makefile,v 1.7 2005/09/24 13:51:47 chs Exp $

.include <bsd.own.mk>

LIB=	Xext
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	DPMS.c MITMisc.c XAppgroup.c XEVI.c XLbx.c XMultibuf.c XSecurity.c \
	XShape.c XShm.c XSync.c XTestExt1.c Xcup.c Xdbe.c extutil.c globals.c

CPPFLAGS+=	${X11FLAGS.THREADS}
CPPFLAGS+=	-I${X11SRCDIR.xc}/lib/X11

LIBDPLIBS=\
	X11	${.CURDIR}/../X11/dynamic

.include <bsd.x11.mk>
.include <bsd.lib.mk>
