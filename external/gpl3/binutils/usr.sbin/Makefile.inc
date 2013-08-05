#	$NetBSD: Makefile.inc,v 1.4 2013/08/05 00:17:52 matt Exp $

BINDIR?=	/usr/sbin
WARNS?=		1

LDADD+=-lz
.ifndef HOSTPROG
BINUTILS_MACHINE_ARCH=${MACHINE_ARCH:C/armv[457]/arm/}
DPADD+=${LIBZ}
.endif
