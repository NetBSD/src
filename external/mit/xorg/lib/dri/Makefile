# $NetBSD: Makefile,v 1.9 2009/02/14 10:48:49 abs Exp $

SUBDIR=	libmesa .WAIT

.if ${MACHINE} == "alpha"
SUBDIR+=	mach64 mga r128 r200 r300 radeon s3v savage sis tdfx trident
.elif ${MACHINE} == "macppc"
SUBDIR+=	mach64 mga r128 r200 r300 radeon tdfx
.elif ${MACHINE} == "sparc64" || ${MACHINE} == "sparc"
SUBDIR+=	mach64 mga r128 r200 r300 radeon
.elif ${MACHINE} == "shark" || ${MACHINE} == "ibmnws" || \
      ${MACHINE} == "sgimips" || ${MACHINE} == "netwinder"
SUBDIR=
+.elif ${MACHINE} == "amd64" || ${MACHINE} == "i386"
SUBDIR+=	i810 i915 i965 mach64 mga r128 r200 r300 radeon s3v \
	savage sis tdfx trident unichrome
.endif

.include <bsd.subdir.mk>
