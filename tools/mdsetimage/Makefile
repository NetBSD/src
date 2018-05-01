#	$NetBSD: Makefile,v 1.16 2018/05/01 12:38:40 christos Exp $

.include <bsd.init.mk>

HOSTPROGNAME=	${MACHINE_GNU_PLATFORM}-mdsetimage

HOST_SRCDIR=	external/gpl3/binutils/usr.sbin/mdsetimage

TOOLCHAINOBJ!=	cd ${.CURDIR}/../binutils && ${PRINTOBJDIR}
BFDDIR?=	${TOOLCHAINOBJ}/build/bfd
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
