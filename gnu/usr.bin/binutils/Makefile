#	$NetBSD: Makefile,v 1.20 2002/09/16 16:22:16 thorpej Exp $

.include <bsd.own.mk>

.if ${MKBFD} != "no" && exists(${.CURDIR}/common/arch/${MACHINE_ARCH}/defs.mk)
.include "${.CURDIR}/common/arch/${MACHINE_ARCH}/defs.mk"

SUBDIR=		common gas gprof ld ${G_PROGRAMS:C/-new$//:C/sysdump//:C/srconv//:cxxfilt=c++filt}
SUBDIR:=	${SUBDIR:O} # alphabetize
.endif	# MKBFD != no

.include <bsd.subdir.mk>
