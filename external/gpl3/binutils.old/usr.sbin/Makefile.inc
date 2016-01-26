#	$NetBSD: Makefile.inc,v 1.2 2016/01/26 19:38:11 christos Exp $

.include "${.PARSEDIR}/../Makefile.inc"
BINDIR?=	/usr/sbin
WARNS?=		1

CPPFLAGS+=-DPACKAGE -DPACKAGE_VERSION
LDADD+=-lz
.ifndef HOSTPROG
BINUTILS_MACHINE_ARCH=${MACHINE_ARCH:C/armv[4-7]/arm/}
DPADD+=${LIBZ}
.endif
