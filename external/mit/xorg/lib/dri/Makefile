# $NetBSD: Makefile,v 1.2 2008/10/30 00:52:43 macallan Exp $

.if ${MACHINE_ARCH} == "powerpc"
SUBDIR= mach64 mga r128 r200 r300 radeon tdfx
.elif ${MACHINE} == "sparc64" || ${MACHINE} == "sparc"
SUBDIR= mach64 mga r128 r200 r300 radeon
.else
SUBDIR=	i810 i915 i965 mach64 mga r128 r200 r300 radeon s3v \
	savage sis tdfx trident unichrome
.endif

.include <bsd.subdir.mk>
