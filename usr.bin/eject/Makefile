#	$NetBSD: Makefile,v 1.5 2009/01/16 17:31:22 christos Exp $

.include <bsd.own.mk>

AMD_SUPPORT=no	# XXX: Off since we don't have the sync umount patch in yet.

PROG=	eject
SRCS=	eject.c

.if ${AMD_SUPPORT:Uno} == "yes"
IDIST=	${NETBSDSRCDIR}/external/bsd/am-utils/dist
.PATH:	${IDIST}/libamu

SRCS+=		am_glue.c
SRCS+=		nfs_prot_xdr.c

CPPFLAGS+=	-DAMD_SUPPORT
CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-I${IDIST}
CPPFLAGS+=	-I${IDIST}/libamu
CPPFLAGS+=	-I${IDIST}/include
.endif

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

WARNS=	4

.include <bsd.prog.mk>
