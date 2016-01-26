#	$NetBSD: Makefile.inc,v 1.1.1.1 2016/01/26 17:27:00 christos Exp $

.if !defined(__COMMON_MAKEFILE_INC_INCLUDED__)
__COMMON_MAKEFILE_INC_INCLUDED__=1

BINUTILS_MACHINE_ARCH=${MACHINE_ARCH:C/armv[4-7]/arm/}
DIST=		${NETBSDSRCDIR}/external/gpl3/binutils/dist
BFDSUBDIR=	lib

.endif
