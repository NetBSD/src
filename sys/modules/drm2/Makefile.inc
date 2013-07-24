# $NetBSD: Makefile.inc,v 1.1.2.2 2013/07/24 03:10:51 riastradh Exp $

# Common makefile cruft for drm2 code.

CPPFLAGS+=	-I${S}/external/bsd/drm2/include
CPPFLAGS+=	-I${S}/external/bsd/drm2/dist/uapi
CPPFLAGS+=	-I${S}/external/bsd/drm2/dist/include
CPPFLAGS+=	-D__KERNEL__

# XXX Is this the right place to set this?
CPPFLAGS+=	-DDIAGNOSTIC

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
CPPFLAGS+=	-DMTRR
CPPFLAGS+=	-DCONFIG_MTRR
.endif
