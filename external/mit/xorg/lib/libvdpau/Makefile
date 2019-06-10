#	$NetBSD: Makefile,v 1.4.2.2 2019/06/10 22:02:42 christos Exp $

.include <bsd.own.mk>

LIB=	vdpau

.PATH:	${X11SRCDIR.vdpau}/src

SRCS=	vdpau_wrapper.c \
	mesa_dri2.c

VDPAU_SYSCONFDIR=	${X11ETCDIR}/vdpau
VDPAU_MODULEDIR=	${X11LIBDIR}/vdpau

CPPFLAGS+=	-DVDPAU_SYSCONFDIR=\"${VDPAU_SYSCONFDIR}\" \
		-DVDPAU_MODULEDIR=\"${VDPAU_MODULEDIR}\"

LIBDPLIBS=	Xext	${.CURDIR}/../libXext \
		X11	${.CURDIR}/../libX11/dynamic

SHLIB_MAJOR=	1
SHLIB_MINOR=	0

PKGDIST=	vdpau

.include <bsd.x11.mk>
.include <bsd.lib.mk>
