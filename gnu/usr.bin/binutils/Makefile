#	$NetBSD: Makefile,v 1.10.8.1 2000/10/25 16:30:31 tv Exp $

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
