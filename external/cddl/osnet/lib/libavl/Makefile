#	$NetBSD: Makefile,v 1.4.28.1 2016/08/06 00:18:41 pgoyette Exp $

.include <bsd.init.mk>

.PATH:	${.CURDIR}/../../dist/common/avl

CPPFLAGS+=-Wall -Wno-unknown-pragmas

LIB=	avl
SRCS=	avl.c

.include "../../Makefile.zfs"
.include <bsd.lib.mk>
