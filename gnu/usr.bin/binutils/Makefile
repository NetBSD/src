#	$NetBSD: Makefile,v 1.22 2003/12/04 09:29:33 mrg Exp $

.include <bsd.own.mk>

.if ${MKBFD} != "no" && exists(${.CURDIR}/common/arch/${MACHINE_ARCH}/defs.mk)
.include "${.CURDIR}/common/arch/${MACHINE_ARCH}/defs.mk"

#SUBDIR=		libopcodes libbfd libiberty .WAIT

SSUBDIR+=	common gas gprof ld ${G_PROGRAMS:C/-new$//:C/sysdump//:C/srconv//:cxxfilt=c++filt}
SUBDIR+=	${SSUBDIR:O} # alphabetize

.endif	# MKBFD != no

.include <bsd.subdir.mk>
