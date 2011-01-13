#	$NetBSD: Makefile,v 1.6 2011/01/13 23:18:24 haad Exp $

.PATH:  ${.CURDIR}/../../dist/lib/libnvpair
.PATH:  ${.CURDIR}/../../dist/common/nvpair

CPPFLAGS+=-Wall -Wno-unknown-pragmas

LIB=	nvpair

SRCS=	libnvpair.c nvpair_alloc_system.c nvpair_alloc_fixed.c nvpair.c

.include "../../Makefile.zfs"
.include <bsd.lib.mk>
