#	$NetBSD: Makefile.inc,v 1.1.12.1 2014/08/19 23:53:01 tls Exp $

.if !defined(__COMMON_MAKEFILE_INC_INCLUDED__)
__COMMON_MAKEFILE_INC_INCLUDED__=1

BINUTILS_MACHINE_ARCH=${MACHINE_ARCH:C/armv[4-7]/arm/}
DIST=		${NETBSDSRCDIR}/external/gpl3/binutils/dist
BFDSUBDIR=	lib

.endif
