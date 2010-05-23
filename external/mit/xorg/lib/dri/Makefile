# $NetBSD: Makefile,v 1.14 2010/05/23 21:31:52 mrg Exp $

SUBDIR=		libmesa .WAIT

.if ${MACHINE_ARCH} == "alpha"
SUBDIR+=	mach64 mga r128 r200 r300 r600 radeon savage sis tdfx
.elif ${MACHINE} == "macppc"
SUBDIR+=	mach64 mga r128 r200 r300 r600 radeon tdfx
.elif ${MACHINE_ARCH} == "sparc64" || ${MACHINE_ARCH} == "sparc"
SUBDIR+=	mach64 mga r128 r200 r300 r600 radeon
.elif ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SUBDIR+=	i810 i915 i965 mach64 mga r128 r200 r300 r600 radeon \
	savage sis tdfx unichrome
.endif

SUBDIR+=	swrast

.include <bsd.subdir.mk>
