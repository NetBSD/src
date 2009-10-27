#	$NetBSD: Makefile,v 1.8 2009/10/27 14:20:21 tsutsui Exp $

NOMAN=	# defined

.include <bsd.own.mk>

HOSTPROGNAME=	${MACHINE_GNU_PLATFORM}-mdsetimage

.if $(HAVE_BINUTILS) == "219"
HOST_SRCDIR=	external/gpl3/binutils/usr.sbin/mdsetimage
.else
HOST_SRCDIR=	gnu/usr.sbin/mdsetimage
.endif

TOOLCHAINOBJ!=	cd ${.CURDIR}/../binutils && ${PRINTOBJDIR}
BFDDIR=		${TOOLCHAINOBJ}/build/bfd
IBERTYDIR=	${TOOLCHAINOBJ}/build/libiberty

.include "${.CURDIR}/../Makefile.host"
