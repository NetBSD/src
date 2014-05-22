#	$NetBSD: Makefile.inc,v 1.1.6.1 2014/05/22 15:54:48 yamt Exp $

.if !defined(__COMMON_MAKEFILE_INC_INCLUDED__)
__COMMON_MAKEFILE_INC_INCLUDED__=1

BINUTILS_MACHINE_ARCH=${MACHINE_ARCH:C/armv[4-7]/arm/}
DIST=		${NETBSDSRCDIR}/external/gpl3/binutils/dist
BFDSUBDIR=	lib

.endif
