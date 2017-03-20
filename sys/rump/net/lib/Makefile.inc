#	$NetBSD: Makefile.inc,v 1.6.2.1 2017/03/20 06:57:53 pgoyette Exp $
#

RUMPTOP=	${.CURDIR}/../../..

CWARNFLAGS.clang+=	-Wno-error=address-of-packed-member -Werror=constant-conversion

.include "${RUMPTOP}/Makefile.rump"
