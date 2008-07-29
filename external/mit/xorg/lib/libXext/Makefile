#	$NetBSD: Makefile,v 1.1.1.1 2008/07/29 05:01:22 mrg Exp $

.include <bsd.own.mk>

LIB=	Xext
.PATH:	${X11SRCDIR.${LIB}}/src

SRCS=	DPMS.c MITMisc.c XAppgroup.c XEVI.c XLbx.c XMultibuf.c XSecurity.c \
	XShape.c XShm.c XSync.c XTestExt1.c Xcup.c Xdbe.c extutil.c globals.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}
#CPPFLAGS+=	-I${X11SRCDIR.xc/lib/X11

LIBDPLIBS=	X11	${.CURDIR}/../libX11/dynamic

.include <bsd.x11.mk>
.include <bsd.lib.mk>
