#	$NetBSD: Makefile,v 1.4 2010/05/22 22:43:57 mrg Exp $

# install include/drm files

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.drm}/include/drm
.PATH:	${X11SRCDIR.drm}/intel
.PATH:	${X11SRCDIR.drm}/radeon
.PATH:	${X11SRCDIR.drm}/libkms

INCS=	drm.h \
	drm_mode.h \
	drm_sarea.h \
	i915_drm.h \
	intel_bufmgr.h \
	mach64_drm.h \
	mga_drm.h \
	nouveau_drm.h \
	r128_drm.h \
	radeon_bo.h \
	radeon_bo_gem.h \
	radeon_bo_int.h \
	radeon_cs.h \
	radeon_cs_gem.h \
	radeon_cs_int.h \
	radeon_drm.h \
	savage_drm.h \
	sis_drm.h \
	via_drm.h \
	libkms.h

INCSDIR=${X11INCDIR}/libdrm

.include <bsd.x11.mk>
.include <bsd.prog.mk>
