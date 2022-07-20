# $NetBSD: Makefile.inc,v 1.9 2022/07/20 01:11:39 riastradh Exp $

# Common makefile cruft for drm2 code.

# XXX All the -D crap needs to go in a header file instead so changing
# it correctly triggers rebuilds.

COPTS+=		-fwrapv

# Overrides first.
CPPFLAGS+=	-I${S}/external/bsd/drm2/include
CPPFLAGS+=	-I${S}/external/bsd/drm2/include/drm

CPPFLAGS+=	-I${S}/external/bsd/common/include

# Then upstream.
CPPFLAGS+=	-I${S}/external/bsd/drm2/dist/include
CPPFLAGS+=	-I${S}/external/bsd/drm2/dist/include/drm
CPPFLAGS+=	-I${S}/external/bsd/drm2/dist/include/uapi

CPPFLAGS+=	-D__KERNEL__

.if ${MACHINE_ARCH} == "i386"
CPPFLAGS+=	-DCONFIG_X86
CPPFLAGS+=	-DCONFIG_X86_PAT
.elif ${MACHINE_ARCH} == "x86_64"
CPPFLAGS+=	-DCONFIG_X86
CPPFLAGS+=	-DCONFIG_X86_64
CPPFLAGS+=	-DCONFIG_X86_PAT
.elif ${MACHINE} == "evbarm"
CPPFLAGS+=	-DCONFIG_ARM
.elif ${MACHINE_ARCH} == "aarch64"
CPPFLAGS+=	-DCONFIG_ARM64
.elif ${MACHINE_ARCH} == "alpha"
CPPFLAGS+=	-Ualpha
.endif

CPPFLAGS+=	-DCONFIG_BACKLIGHT_CLASS_DEVICE=0
CPPFLAGS+=	-DCONFIG_BACKLIGHT_CLASS_DEVICE_MODULE=0
CPPFLAGS+=	-DCONFIG_DRM_FBDEV_EMULATION=1
CPPFLAGS+=	-DCONFIG_DRM_FBDEV_OVERALLOC=100
CPPFLAGS+=	-DCONFIG_FB=0
CPPFLAGS+=	-DCONFIG_LOCKDEP=0
CPPFLAGS+=	-DCONFIG_PCI=1

CWARNFLAGS+=	-Wno-missing-field-initializers

# Needed for drmums modules.
CPPFLAGS+=	-DCONFIG_DRM_LEGACY

# XXX Is this the right place to set this?
CPPFLAGS+=	-DDIAGNOSTIC
