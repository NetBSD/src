#	$NetBSD: Makefile,v 1.12.2.1 2018/05/02 07:20:25 pgoyette Exp $

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
