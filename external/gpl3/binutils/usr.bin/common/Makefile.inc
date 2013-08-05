#	$NetBSD: Makefile.inc,v 1.2 2013/08/05 00:08:58 matt Exp $

.if !defined(__COMMON_MAKEFILE_INC_INCLUDED__)
__COMMON_MAKEFILE_INC_INCLUDED__=1

BINUTILS_MACHINE_ARCH=${MACHINE_ARCH:C/armv[467]/arm/}
DIST=		${NETBSDSRCDIR}/external/gpl3/binutils/dist
BFDSUBDIR=	lib

.endif
