#	$NetBSD: Makefile,v 1.21 2003/11/27 15:02:21 mrg Exp $

.include <bsd.own.mk>

.if ${MKBFD} != "no" && exists(${.CURDIR}/common/arch/${MACHINE_ARCH}/defs.mk)
.include "${.CURDIR}/common/arch/${MACHINE_ARCH}/defs.mk"

. if ${HAVE_BINUTILS214} == "yes"
SUBDIR=		libopcodes libbfd libiberty
. endif

SSUBDIR+=	common gas gprof ld ${G_PROGRAMS:C/-new$//:C/sysdump//:C/srconv//:cxxfilt=c++filt}
SUBDIR+=	${SSUBDIR:O} # alphabetize

.endif	# MKBFD != no

.include <bsd.subdir.mk>
