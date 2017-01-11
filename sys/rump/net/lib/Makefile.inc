#	$NetBSD: Makefile.inc,v 1.7 2017/01/11 12:19:44 joerg Exp $
#

RUMPTOP=	${.CURDIR}/../../..

CWARNFLAGS.clang+=	-Wno-error=address-of-packed-member -Werror=constant-conversion

.include "${RUMPTOP}/Makefile.rump"
