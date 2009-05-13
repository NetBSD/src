#	$NetBSD: Makefile,v 1.6.2.1 2009/05/13 19:19:48 jym Exp $

.include <bsd.own.mk>

.if !defined(SMALLPROG)
AMD_SUPPORT?=yes
.endif

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

.include <bsd.prog.mk>
