#	$NetBSD: Makefile.inc,v 1.2.2.1 2016/11/04 14:45:10 pgoyette Exp $

.include "${.PARSEDIR}/../Makefile.inc"
BINDIR?=	/usr/sbin
WARNS?=		1

CPPFLAGS+=-DPACKAGE -DPACKAGE_VERSION
LDADD+=-lz
.ifndef HOSTPROG
BINUTILS_MACHINE_ARCH=${MACHINE_ARCH:C/armv[4-7]/arm/}
DPADD+=${LIBZ}
.endif
