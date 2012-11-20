#	$NetBSD: Makefile,v 1.24.6.2 2012/11/20 18:52:37 matt Exp $

.include <bsd.own.mk>

.if ${MKBINUTILS} != "no" && exists(${.CURDIR}/common/arch/${MACHINE_ARCH}/defs.mk)
.include "${.CURDIR}/common/arch/${MACHINE_ARCH}/defs.mk"

#SUBDIR=		libopcodes libbfd libiberty .WAIT

SSUBDIR+=	common gas gprof ld ${G_PROGRAMS:C/-new$//:C/sysdump//:C/srconv//:cxxfilt=c++filt}
SUBDIR+=	${SSUBDIR:O} # alphabetize

.endif	# MKBINUTILS != no

.include <bsd.subdir.mk>
