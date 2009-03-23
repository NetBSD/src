#	$NetBSD: Makefile,v 1.7 2009/03/23 04:00:44 christos Exp $

.include <bsd.own.mk>

AMD_SUPPORT?=yes

PROG=	eject
SRCS=	eject.c

.if ${AMD_SUPPORT:Uno} == "yes"
IDIST=		${NETBSDSRCDIR}/external/bsd/am-utils/dist
LIBAMUOBJDIR!=	cd ${IDIST}/../lib/libamu && ${PRINTOBJDIR}

.PATH:	${IDIST}/libamu

SRCS+=		am_glue.c
SRCS+=		nfs_prot_xdr.c

CPPFLAGS+=	-DAMD_SUPPORT
CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-I${IDIST}
CPPFLAGS+=	-I${IDIST}/libamu
CPPFLAGS+=	-I${IDIST}/include
CPPFLAGS+=	-I${IDIST}/../include
CPPFLAGS+=	-I${LIBAMUOBJDIR}
.endif

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

WARNS=	4

.include <bsd.prog.mk>
