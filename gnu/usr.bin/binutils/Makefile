#	$NetBSD: Makefile,v 1.12 2001/01/09 03:13:40 lukem Exp $

.if (${MACHINE_ARCH} != sparc64)
SUBDIR=		common .WAIT addr2line ar c++filt nm objcopy objdump ranlib \
		size strings strip

.endif

TEXINFO=	binutils.info
INFOFLAGS=	-I${.CURDIR}/../../dist/binutils
.PATH: ${.CURDIR}/../../dist/binutils

.include <bsd.info.mk>
.include <bsd.subdir.mk>
