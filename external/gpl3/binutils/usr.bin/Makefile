#	$NetBSD: Makefile,v 1.1 2009/08/18 20:22:07 skrll Exp $

.include <bsd.own.mk>

.if ${MKBFD} != "no" && exists(${.CURDIR}/common/arch/${MACHINE_ARCH}/defs.mk)
.include "${.CURDIR}/common/arch/${MACHINE_ARCH}/defs.mk"

SSUBDIR+=	common gas gprof ld ${G_PROGRAMS:C/-new$//:C/sysdump//:C/srconv//:cxxfilt=c++filt}
SUBDIR+=	${SSUBDIR:O} # alphabetize

.endif	# MKBFD != no

.include <bsd.subdir.mk>
