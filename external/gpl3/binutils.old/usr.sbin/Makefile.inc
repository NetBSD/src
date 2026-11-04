#	$NetBSD: Makefile.inc,v 1.9.2.1 2026/11/04 22:46:35 perseant Exp $

.include "${.PARSEDIR}/../Makefile.inc"
BINDIR?=	/usr/sbin
WARNS?=		1

CPPFLAGS+=-DPACKAGE -DPACKAGE_VERSION
LDADD+=-lz
.ifndef HOSTPROG
BINUTILS_MACHINE_ARCH?=${MACHINE_ARCH:C/armv[4-7]/arm/}
DPADD+=${LIBZ}
.endif
