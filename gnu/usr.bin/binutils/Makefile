#	$NetBSD: Makefile,v 1.19 2002/06/18 08:21:58 itojun Exp $

.include <bsd.own.mk>

.if defined(USE_NEW_TOOLCHAIN)

.if ${MKBFD} != "no" && exists(${.CURDIR}/common/arch/${MACHINE_ARCH}/defs.mk)
.include "${.CURDIR}/common/arch/${MACHINE_ARCH}/defs.mk"

SUBDIR=		common gas gprof ld ${G_PROGRAMS:C/-new$//:C/sysdump//:C/srconv//:cxxfilt=c++filt}
SUBDIR:=	${SUBDIR:O} # alphabetize
.endif	# MKBFD != no

.else
#
# XXX - BEGIN - Old toolchain build goop - delete when everything migrated - XXX
#
.if (${MACHINE_ARCH} != sparc64)
SUBDIR=		common .WAIT addr2line ar c++filt nm objcopy objdump ranlib \
		size strings strip

.endif

TEXINFO=	binutils.info
INFOFLAGS=	-I${.CURDIR}/../../dist/binutils
.PATH: ${.CURDIR}/../../dist/binutils

.include <bsd.info.mk>
#
# XXX - END - Old toolchain build goop - delete when everything migrated - XXX
#
.endif

.include <bsd.subdir.mk>
