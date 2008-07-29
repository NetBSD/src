#	$NetBSD: Makefile,v 1.1.1.1 2008/07/29 05:01:21 mrg Exp $

# install include/X11/drm files

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.drm}/shared-core
.PATH:	${X11SRCDIR.drm}/bsd-core

INCS=	drm.h drm_sarea.h i915_drm.h mach64_drm.h mga_drm.h r128_drm.h \
	r300_reg.h radeon_drm.h savage_drm.h sis_drm.h via_3d_reg.h via_drm.h
INCSDIR=${X11INCDIR}/X11/drm

.include <bsd.x11.mk>
.include <bsd.prog.mk>
