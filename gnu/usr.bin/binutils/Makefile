#	$NetBSD: Makefile,v 1.10 1999/02/11 16:15:55 tv Exp $

SUBDIR=		common addr2line ar c++filt nm objcopy objdump ranlib size \
		strings strip

all-addr2line all-ar all-c++filt all-nm all-objcopy all-objdump \
	all-ranlib all-size all-strings all-strip: all-common

TEXINFO=	binutils.info
INFOFLAGS=	-I${.CURDIR}/../../dist/binutils
.PATH: ${.CURDIR}/../../dist/binutils

.include <bsd.info.mk>
.include <bsd.subdir.mk>
