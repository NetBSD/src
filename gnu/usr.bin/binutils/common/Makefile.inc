#	$NetBSD: Makefile.inc,v 1.2.44.1 2013/12/18 18:43:29 matt Exp $

.if !defined(__COMMON_MAKEFILE_INC_INCLUDED__)
__COMMON_MAKEFILE_INC_INCLUDED__=1

DIST=		${.CURDIR}/../../../dist/binutils
BFDSUBDIR=	lib
BFD_MACHINE_ARCH=${MACHINE_ARCH:S/armv7/arm/}

.endif
