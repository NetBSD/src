#	$NetBSD: Makefile,v 1.15 2018/05/01 19:59:46 christos Exp $

.include <bsd.hostinit.mk>

HOSTPROGNAME=	${MACHINE_GNU_PLATFORM}-dbsym

HOST_SRCDIR=    external/gpl3/binutils/usr.sbin/dbsym

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
