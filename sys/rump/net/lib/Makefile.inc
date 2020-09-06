#	$NetBSD: Makefile.inc,v 1.8 2020/09/06 07:20:30 mrg Exp $
#

RUMPTOP=	${.CURDIR}/../../..

CWARNFLAGS.clang+=	-Wno-error=address-of-packed-member -Werror=constant-conversion
CWARNFLAGS.gcc+=	${GCC_NO_ADDR_OF_PACKED_MEMBER}

.include "${RUMPTOP}/Makefile.rump"
