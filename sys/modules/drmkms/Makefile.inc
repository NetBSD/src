# $NetBSD: Makefile.inc,v 1.5 2018/08/27 13:53:42 riastradh Exp $

# Common makefile cruft for drm2 code.

# XXX All the -D crap needs to go in a header file instead so changing
# it correctly triggers rebuilds.

# Overrides first.
CPPFLAGS+=	-I${S}/external/bsd/drm2/include

CPPFLAGS+=	-I${S}/external/bsd/common/include

# Then upstream.
CPPFLAGS+=	-I${S}/external/bsd/drm2/dist/include
CPPFLAGS+=	-I${S}/external/bsd/drm2/dist/include/drm
CPPFLAGS+=	-I${S}/external/bsd/drm2/dist/uapi

# Must come last because some header file names are repeated in
# dist/drm (I'm looking ta you, drm_legacy.h), while others sometimes
# explicitly write <uapi/...>.  (Maybe that should have been imported
# under dist/include/uapi rather than dist/uapi.)
CPPFLAGS+=	-I${S}/external/bsd/drm2/dist

# XXX Should probably be in a header file.  opt_drmkms.h?
CPPFLAGS+=	-D__KERNEL__

CPPFLAGS+=	-DCONFIG_BACKLIGHT_CLASS_DEVICE=0
CPPFLAGS+=	-DCONFIG_BACKLIGHT_CLASS_DEVICE_MODULE=0
CPPFLAGS+=	-DCONFIG_DRM_FBDEV_EMULATION=0
CPPFLAGS+=	-DCONFIG_FB=0

# XXX Is this the right place to set this?
CPPFLAGS+=	-DDIAGNOSTIC
