#	$NetBSD: Makefile,v 1.2 2009/03/25 15:36:08 tsutsui Exp $

HOSTPROG=	macppc_mkboothfs
SRCS=		mkboothfs.c
NOMAN=		# defined

CLEANFILES+=	boothfs

.include <bsd.own.mk>

.ifndef NOCOMPATLIB
COMPATOBJ!=	cd ${NETBSDSRCDIR}/tools/compat && ${PRINTOBJDIR}
.-include       "${COMPATOBJ}/defs.mk"
HOST_CPPFLAGS+=	-I${NETBSDSRCDIR}/tools/compat
.endif

.include <bsd.hostprog.mk>
