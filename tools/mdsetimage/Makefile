#	$NetBSD: Makefile,v 1.4 2001/11/12 23:16:30 tv Exp $

.include <bsd.own.mk>

HOSTPROGNAME=	${MACHINE_GNU_PLATFORM}-mdsetimage
HOST_SRCDIR=	gnu/usr.sbin/mdsetimage

TOOLCHAINOBJ!=	cd ${.CURDIR}/../toolchain && ${PRINTOBJDIR}
BFDDIR=		${TOOLCHAINOBJ}/build/bfd
IBERTYDIR=	${TOOLCHAINOBJ}/build/libiberty

.include "${.CURDIR}/../Makefile.host"
