# $NetBSD: Makefile,v 1.2.2.6 2009/09/17 04:24:29 snj Exp $

SUBDIR=		libmesa .WAIT

.if ${MACHINE_ARCH} == "alpha"
SUBDIR+=	mach64 mga r128 r200 r300 radeon s3v savage sis tdfx trident
.elif ${MACHINE} == "macppc"
SUBDIR+=	mach64 mga r128 r200 r300 radeon tdfx
.elif ${MACHINE_ARCH} == "sparc64" || ${MACHINE_ARCH} == "sparc"
SUBDIR+=	mach64 mga r128 r200 r300 radeon
.elif ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SUBDIR+=	i810 i915 i965 mach64 mga r128 r200 r300 radeon s3v \
	savage sis tdfx trident unichrome
.endif

SUBDIR+=	swrast

.include <bsd.subdir.mk>
