#	$NetBSD: Makefile,v 1.2 2001/11/12 23:16:26 tv Exp $

.include <bsd.own.mk>

HOSTPROGNAME=	${MACHINE_GNU_PLATFORM}-dbsym
HOST_SRCDIR=	gnu/usr.sbin/dbsym

TOOLCHAINOBJ!=	cd ${.CURDIR}/../toolchain && ${PRINTOBJDIR}
BFDDIR=		${TOOLCHAINOBJ}/build/bfd
IBERTYDIR=	${TOOLCHAINOBJ}/build/libiberty

.include "${.CURDIR}/../Makefile.host"
