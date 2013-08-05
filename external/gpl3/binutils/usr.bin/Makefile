#	$NetBSD: Makefile,v 1.4 2013/08/05 00:08:58 matt Exp $

.include <bsd.own.mk>

BINUTILS_MACHINE_ARCH=${MACHINE_ARCH:C/armv[457]/arm/}

.if ${MKBINUTILS} != "no" && exists(${.CURDIR}/common/arch/${BINUTILS_MACHINE_ARCH}/defs.mk)
.include "${.CURDIR}/common/arch/${BINUTILS_MACHINE_ARCH}/defs.mk"

SSUBDIR+=	gas gprof ld ${G_PROGRAMS:C/-new$//:C/sysdump//:C/srconv//:cxxfilt=c++filt}
SUBDIR+=	common .WAIT ${SSUBDIR:O} # alphabetize

.endif	# MKBINUTILS != no

.include <bsd.subdir.mk>
