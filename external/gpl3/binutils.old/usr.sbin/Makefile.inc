#	$NetBSD: Makefile.inc,v 1.4 2016/10/26 17:09:50 christos Exp $

.include "${.PARSEDIR}/../Makefile.inc"
BINDIR?=	/usr/sbin
WARNS?=		1

CPPFLAGS+=-DPACKAGE -DPACKAGE_VERSION
LDADD+=-lz
.ifndef HOSTPROG
BINUTILS_MACHINE_ARCH=${MACHINE_ARCH:C/armv[4-7]/arm/}
DPADD+=${LIBZ}
.endif
