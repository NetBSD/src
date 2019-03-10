#	$NetBSD: Makefile,v 1.1 2019/03/10 11:18:02 mrg Exp $

.include <bsd.own.mk>

LIB=	vdpau

.PATH:	${X11SRCDIR.vdpau}/src

SRCS=	vdpau_wrapper.c \
	mesa_dri2.c

VDPAU_SYSCONFDIR=	${X11ETCDIR}/vdpau
VDPAU_MODULEDIR=	${X11LIBDIR}/vdpau

CFLAGS+=	-DVDPAU_SYSCONFDIR=\"${VDPAU_SYSCONFDIR}\" \
		-DVDPAU_MODULEDIR=\"${VDPAU_MODULEDIR}\"

SHLIB_MAJOR=	1
SHLIB_MINOR=	0

# XXX todo
#PKGDIST=	libvdpau

.include <bsd.x11.mk>
.include <bsd.lib.mk>
