#	$NetBSD: Makefile,v 1.22.44.1 2013/12/18 18:43:29 matt Exp $

.include <bsd.own.mk>
.include "Makefile.inc"

.if ${MKBFD} != "no" && exists(${.CURDIR}/common/arch/${BFD_MACHINE_ARCH}/defs.mk)
.include "${.CURDIR}/common/arch/${BFD_MACHINE_ARCH}/defs.mk"

#SUBDIR=		libopcodes libbfd libiberty .WAIT

SSUBDIR+=	common gas gprof ld ${G_PROGRAMS:C/-new$//:C/sysdump//:C/srconv//:cxxfilt=c++filt}
SUBDIR+=	${SSUBDIR:O} # alphabetize

.endif	# MKBFD != no

.include <bsd.subdir.mk>
