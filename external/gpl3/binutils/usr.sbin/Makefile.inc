#	$NetBSD: Makefile.inc,v 1.5 2013/08/06 05:35:57 matt Exp $

BINDIR?=	/usr/sbin
WARNS?=		1

LDADD+=-lz
.ifndef HOSTPROG
BINUTILS_MACHINE_ARCH=${MACHINE_ARCH:C/armv[4-7]/arm/}
DPADD+=${LIBZ}
.endif
