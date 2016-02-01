#	$NetBSD: Makefile,v 1.10 2016/02/01 13:44:52 christos Exp $

NOMAN=	# defined

.include <bsd.own.mk>

HOSTPROGNAME=	${MACHINE_GNU_PLATFORM}-mdsetimage

HOST_SRCDIR=	external/gpl3/binutils/usr.sbin/mdsetimage

TOOLCHAINOBJ!=	cd ${.CURDIR}/../binutils && ${PRINTOBJDIR}
BFDDIR=		${TOOLCHAINOBJ}/build/bfd
IBERTYDIR=	${TOOLCHAINOBJ}/build/libiberty

.include "${.CURDIR}/../Makefile.host"

LIBDL!=		grep -- -ldl ${TOOLCHAINOBJ}/build/ld/Makefile

.if !empty(LIBDL)
LDADD+=	-ldl
.endif
