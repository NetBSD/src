#	$NetBSD: Makefile,v 1.18 2024/06/30 16:12:13 christos Exp $

.include <bsd.hostinit.mk>

HOSTPROGNAME=	${MACHINE_GNU_PLATFORM}-mdsetimage

HOST_SRCDIR=	external/gpl3/binutils/usr.sbin/mdsetimage

TOOLCHAINOBJ!=	cd ${.CURDIR}/../binutils && ${PRINTOBJDIR}
BFDDIR?=	${TOOLCHAINOBJ}/build/bfd
SFRAMEDIR?=	${TOOLCHAINOBJ}/build/libsframe
IBERTYDIR?=	${TOOLCHAINOBJ}/build/libiberty

.include "${.CURDIR}/../Makefile.host"


LDMAKEFILE=${TOOLCHAINOBJ}/build/ld/Makefile
.if exists(${LDMAKEFILE})
HAVE_LIBDL!=	grep -- -ldl ${LDMAKEFILE} || echo no
.else
HAVE_LIBDL=	no
.endif

.if ${HAVE_LIBDL} != "no"
LDADD+=	-ldl
.endif
