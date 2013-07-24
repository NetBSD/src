# $NetBSD: Makefile.inc,v 1.1.2.3 2013/07/24 03:17:03 riastradh Exp $

# Common makefile cruft for drm2 code.

CPPFLAGS+=	-I${S}/external/bsd/drm2/include
CPPFLAGS+=	-I${S}/external/bsd/drm2/dist/uapi
CPPFLAGS+=	-I${S}/external/bsd/drm2/dist/include
CPPFLAGS+=	-D__KERNEL__

# XXX Is this the right place to set this?
CPPFLAGS+=	-DDIAGNOSTIC

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
CPPFLAGS+=	-DCONFIG_MTRR
CPPFLAGS+=	-DCONFIG_X86
CPPFLAGS+=	-DMTRR
.endif
