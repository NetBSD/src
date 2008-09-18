#	$NetBSD: Makefile,v 1.3.2.2 2008/09/18 05:18:37 wrstuden Exp $

.include <bsd.own.mk>

LIB=	Xext
.PATH:	${X11SRCDIR.${LIB}}/src

SRCS=	DPMS.c MITMisc.c XAppgroup.c XEVI.c XLbx.c XMultibuf.c XSecurity.c \
	XShape.c XShm.c XSync.c XTestExt1.c Xcup.c Xdbe.c extutil.c globals.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}

LIBDPLIBS=	X11	${.CURDIR}/../libX11/dynamic

# XXX
COPTS.extutil.c+=	-Wno-error

.include <bsd.x11.mk>
.include <bsd.lib.mk>
