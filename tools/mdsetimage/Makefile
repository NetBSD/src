#	$NetBSD: Makefile,v 1.7 2003/12/04 09:59:07 mrg Exp $

NOMAN=	# defined

.include <bsd.own.mk>

HOSTPROGNAME=	${MACHINE_GNU_PLATFORM}-mdsetimage
HOST_SRCDIR=	gnu/usr.sbin/mdsetimage

TOOLCHAINOBJ!=	cd ${.CURDIR}/../binutils && ${PRINTOBJDIR}
BFDDIR=		${TOOLCHAINOBJ}/build/bfd
IBERTYDIR=	${TOOLCHAINOBJ}/build/libiberty

.include "${.CURDIR}/../Makefile.host"
