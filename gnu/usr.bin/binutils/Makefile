#	$NetBSD: Makefile,v 1.13 2001/08/06 19:11:59 tv Exp $

.include <bsd.own.mk>

.if defined(USE_NEW_TOOLCHAIN)

.include "${.CURDIR}/common/arch/${MACHINE_ARCH}/defs.mk"

SUBDIR=		common gas ld ${G_PROGRAMS:C/-new$//:cxxfilt=c++filt}
SUBDIR:=	${SUBDIR:O} # alphabetize

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
