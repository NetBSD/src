#	$NetBSD: Makefile.inc,v 1.4.20.1 2017/12/03 11:39:17 jdolecek Exp $
#

RUMPTOP=	${.CURDIR}/../../..

CWARNFLAGS.clang+=	-Wno-error=address-of-packed-member -Werror=constant-conversion

.include "${RUMPTOP}/Makefile.rump"
