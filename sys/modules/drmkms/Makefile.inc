# $NetBSD: Makefile.inc,v 1.3.2.1 2014/08/10 06:56:10 tls Exp $

# Common makefile cruft for drm2 code.

# XXX All the -D crap needs to go in a header file instead so changing
# it correctly triggers rebuilds.

CPPFLAGS+=	-I${S}/external/bsd/drm2/include
CPPFLAGS+=	-I${S}/external/bsd/drm2/dist
CPPFLAGS+=	-I${S}/external/bsd/drm2/dist/include
CPPFLAGS+=	-I${S}/external/bsd/drm2/dist/include/drm
CPPFLAGS+=	-I${S}/external/bsd/drm2/dist/uapi
CPPFLAGS+=	-I${S}/external/bsd/common/include
CPPFLAGS+=	-D__KERNEL__

# XXX Is this the right place to set this?
CPPFLAGS+=	-DDIAGNOSTIC

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
CPPFLAGS+=	-DCONFIG_MTRR
CPPFLAGS+=	-DCONFIG_X86
.endif
