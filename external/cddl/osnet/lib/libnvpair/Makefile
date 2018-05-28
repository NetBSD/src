#	$NetBSD: Makefile,v 1.9 2018/05/28 21:05:09 chs Exp $

.include <bsd.init.mk>

.PATH:  ${.CURDIR}/../../dist/lib/libnvpair
.PATH:  ${.CURDIR}/../../dist/common/nvpair
.PATH:  ${.CURDIR}/../../dist/uts/common/rpc

CPPFLAGS+=	-I${OSNETDIR}/sys \
		-I${OPENSOLARIS_USR_DISTDIR}/head \
		-I${OPENSOLARIS_SYS_DISTDIR}/common \
		-I${OPENSOLARIS_SYS_DISTDIR}/uts/common

CPPFLAGS+=	-Wall \
		-Wno-unknown-pragmas \
		-Wno-shadow \
		-Wno-format-security \
		-Wno-format-nonliteral \
		-Wno-cast-qual

LIB=	nvpair

SRCS=	libnvpair.c nvpair_alloc_system.c nvpair_alloc_fixed.c nvpair.c fnvpair.c nvpair_json.c
SRCS+=	xdr.c
SRCS+=	xdr_array.c
SRCS+=	xdr_mem.c

.include "../../Makefile.zfs"
.include <bsd.lib.mk>
