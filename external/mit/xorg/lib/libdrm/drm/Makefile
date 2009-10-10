#	$NetBSD: Makefile,v 1.1.1.1.8.2 2009/10/10 19:53:22 sborrill Exp $

# install include/drm files

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.drm}/shared-core
.PATH:	${X11SRCDIR.drm}/bsd-core

INCS=	drm.h drm_sarea.h drm_mode.h \
	i915_drm.h mach64_drm.h mga_drm.h nouveau_drm.h \
	r128_drm.h r300_reg.h radeon_drm.h \
	savage_drm.h sis_drm.h via_3d_reg.h via_drm.h \
	xgi_drm.h
INCSDIR=${X11INCDIR}/drm

.include <bsd.x11.mk>
.include <bsd.prog.mk>
