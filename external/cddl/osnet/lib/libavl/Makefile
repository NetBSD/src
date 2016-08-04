#	$NetBSD: Makefile,v 1.5 2016/08/04 17:07:24 christos Exp $

.include <bsd.init.mk>

.PATH:	${.CURDIR}/../../dist/common/avl

CPPFLAGS+=-Wall -Wno-unknown-pragmas

LIB=	avl
SRCS=	avl.c

.include "../../Makefile.zfs"
.include <bsd.lib.mk>
