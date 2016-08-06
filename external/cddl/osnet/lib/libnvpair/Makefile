#	$NetBSD: Makefile,v 1.7.2.1 2016/08/06 00:18:41 pgoyette Exp $

.include <bsd.init.mk>

.PATH:  ${.CURDIR}/../../dist/lib/libnvpair
.PATH:  ${.CURDIR}/../../dist/common/nvpair
.PATH:  ${.CURDIR}/../../dist/uts/common/rpc

CPPFLAGS+=-Wall -Wno-unknown-pragmas

LIB=	nvpair

SRCS=	libnvpair.c nvpair_alloc_system.c nvpair_alloc_fixed.c nvpair.c
SRCS+=	xdr.c
SRCS+=	xdr_array.c
SRCS+=	xdr_mem.c

.include "../../Makefile.zfs"
.include <bsd.lib.mk>
