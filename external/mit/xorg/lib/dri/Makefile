# $NetBSD: Makefile,v 1.7 2008/11/24 07:51:51 rtr Exp $

SUBDIR=	libmesa .WAIT

.if ${MACHINE} == "macppc"
SUBDIR+=	mach64 mga r128 r200 r300 radeon tdfx
.elif ${MACHINE} == "sparc64" || ${MACHINE} == "sparc"
SUBDIR+=	mach64 mga r128 r200 r300 radeon
.elif ${MACHINE} == "shark" || ${MACHINE} == "ibmnws" || \
      ${MACHINE} == "sgimips" || ${MACHINE} == "netwinder"
SUBDIR=
.else
SUBDIR+=	i810 i915 i965 mach64 mga r128 r200 r300 radeon s3v \
	savage sis tdfx trident unichrome
.endif

.include <bsd.subdir.mk>
