#	$NetBSD: Makefile,v 1.3 2001/12/13 03:44:37 lukem Exp $

NOMAN=	# defined

.include <bsd.own.mk>

HOSTPROGNAME=	${MACHINE_GNU_PLATFORM}-dbsym
HOST_SRCDIR=	gnu/usr.sbin/dbsym

TOOLCHAINOBJ!=	cd ${.CURDIR}/../toolchain && ${PRINTOBJDIR}
BFDDIR=		${TOOLCHAINOBJ}/build/bfd
IBERTYDIR=	${TOOLCHAINOBJ}/build/libiberty

.include "${.CURDIR}/../Makefile.host"
