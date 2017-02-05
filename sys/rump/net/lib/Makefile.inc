#	$NetBSD: Makefile.inc,v 1.4.38.3 2017/02/05 13:41:00 skrll Exp $
#

RUMPTOP=	${.CURDIR}/../../..

CWARNFLAGS.clang+=	-Wno-error=address-of-packed-member -Werror=constant-conversion

.include "${RUMPTOP}/Makefile.rump"
