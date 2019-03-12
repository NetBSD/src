#	$NetBSD: Makefile,v 1.3 2019/03/12 11:05:09 christos Exp $

.include <bsd.own.mk>

LIB=	vdpau

.PATH:	${X11SRCDIR.vdpau}/src

SRCS=	vdpau_wrapper.c \
	mesa_dri2.c

VDPAU_SYSCONFDIR=	${X11ETCDIR}/vdpau
VDPAU_MODULEDIR=	${X11LIBDIR}/vdpau

CPPFLAGS+=	-DVDPAU_SYSCONFDIR=\"${VDPAU_SYSCONFDIR}\" \
		-DVDPAU_MODULEDIR=\"${VDPAU_MODULEDIR}\"

SHLIB_MAJOR=	1
SHLIB_MINOR=	0

PKGDIST=	vdpau

.include <bsd.x11.mk>
.include <bsd.lib.mk>
