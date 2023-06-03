#	$NetBSD: Makefile.inc,v 1.10 2023/06/03 21:26:29 lukem Exp $
#

RUMPTOP=	${.CURDIR}/../../..

CWARNFLAGS.clang+=	-Werror=constant-conversion
CWARNFLAGS=		${CC_WNO_ADDRESS_OF_PACKED_MEMBER}

.include "${RUMPTOP}/Makefile.rump"
