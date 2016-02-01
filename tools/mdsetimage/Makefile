#	$NetBSD: Makefile,v 1.11 2016/02/01 14:17:21 christos Exp $

NOMAN=	# defined

.include <bsd.own.mk>

HOSTPROGNAME=	${MACHINE_GNU_PLATFORM}-mdsetimage

HOST_SRCDIR=	external/gpl3/binutils/usr.sbin/mdsetimage

TOOLCHAINOBJ!=	cd ${.CURDIR}/../binutils && ${PRINTOBJDIR}
BFDDIR=		${TOOLCHAINOBJ}/build/bfd
IBERTYDIR=	${TOOLCHAINOBJ}/build/libiberty

.include "${.CURDIR}/../Makefile.host"

LIBDL!=		grep -- -ldl ${TOOLCHAINOBJ}/build/ld/Makefile || echo no

.if ${LIBDL} != "no"
LDADD+=	-ldl
.endif
