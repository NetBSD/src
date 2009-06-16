# $NetBSD: Makefile,v 1.12 2009/06/16 00:26:11 mrg Exp $

.if ${MACHINE_ARCH} == "alpha"
SUBDIR=		libmesa .WAIT
SUBDIR+=	mach64 mga r128 r200 r300 radeon s3v savage sis tdfx trident
.elif ${MACHINE} == "macppc"
SUBDIR=         libmesa .WAIT
SUBDIR+=	mach64 mga r128 r200 r300 radeon tdfx
.elif ${MACHINE_ARCH} == "sparc64" || ${MACHINE_ARCH} == "sparc"
SUBDIR=         libmesa .WAIT
SUBDIR+=	mach64 mga r128 r200 r300 radeon
.elif ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SUBDIR=         libmesa .WAIT
SUBDIR+=	i810 i915 i965 mach64 mga r128 r200 r300 radeon s3v \
	savage sis tdfx trident unichrome
.endif

.if !empty(SUBDIR)
SUBDIR+=	swrast
.endif

.include <bsd.subdir.mk>
