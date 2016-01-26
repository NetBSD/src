#	$NetBSD: Makefile.inc,v 1.1.1.1 2016/01/26 17:27:00 christos Exp $

BINDIR?=	/usr/sbin
WARNS?=		1

CPPFLAGS+=-DPACKAGE -DPACKAGE_VERSION
LDADD+=-lz
.ifndef HOSTPROG
BINUTILS_MACHINE_ARCH=${MACHINE_ARCH:C/armv[4-7]/arm/}
DPADD+=${LIBZ}
.endif
