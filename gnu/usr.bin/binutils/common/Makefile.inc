#	$NetBSD: Makefile.inc,v 1.1 2003/11/27 15:02:21 mrg Exp $

.if !defined(__COMMON_MAKEFILE_INC_INCLUDED__)
__COMMON_MAKEFILE_INC_INCLUDED__=1

.if ${HAVE_BINUTILS214} == "yes"
DIST=		${.CURDIR}/../../../dist/binutils
BFDSUBDIR=	usr.bin/binutils
.else
DIST=		${.CURDIR}/../../../dist/toolchain
BFDSUBDIR=	lib
.endif

.endif
