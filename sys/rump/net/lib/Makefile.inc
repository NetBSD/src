#	$NetBSD: Makefile.inc,v 1.9 2023/06/03 08:52:59 lukem Exp $
#

RUMPTOP=	${.CURDIR}/../../..

CWARNFLAGS.clang+=	-Wno-error=address-of-packed-member -Werror=constant-conversion
CWARNFLAGS.gcc+=	${CC_WNO_ADDRESS_OF_PACKED_MEMBER}

.include "${RUMPTOP}/Makefile.rump"
