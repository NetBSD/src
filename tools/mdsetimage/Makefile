#	$NetBSD: Makefile,v 1.6 2003/07/25 16:26:54 mrg Exp $

NOMAN=	# defined

.include <bsd.own.mk>

HOSTPROGNAME=	${MACHINE_GNU_PLATFORM}-mdsetimage
HOST_SRCDIR=	gnu/usr.sbin/mdsetimage

.if ${USE_TOOLS_TOOLCHAIN} != "yes"
TOOLCHAINOBJ!=	cd ${.CURDIR}/../binutils && ${PRINTOBJDIR}
.else
TOOLCHAINOBJ!=	cd ${.CURDIR}/../toolchain && ${PRINTOBJDIR}
.endif
BFDDIR=		${TOOLCHAINOBJ}/build/bfd
IBERTYDIR=	${TOOLCHAINOBJ}/build/libiberty

.include "${.CURDIR}/../Makefile.host"
