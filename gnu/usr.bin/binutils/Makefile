#	$NetBSD: Makefile,v 1.11 2000/10/25 13:00:59 mrg Exp $

.if (${MACHINE_ARCH} != sparc64)
SUBDIR=		common addr2line ar c++filt nm objcopy objdump ranlib size \
		strings strip

all-addr2line all-ar all-c++filt all-nm all-objcopy all-objdump \
	all-ranlib all-size all-strings all-strip: all-common
.endif

TEXINFO=	binutils.info
INFOFLAGS=	-I${.CURDIR}/../../dist/binutils
.PATH: ${.CURDIR}/../../dist/binutils

.include <bsd.info.mk>
.include <bsd.subdir.mk>
