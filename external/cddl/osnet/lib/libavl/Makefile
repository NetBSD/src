#	$NetBSD: Makefile,v 1.5.14.1 2020/04/13 07:56:38 martin Exp $

.include <bsd.init.mk>

USE_SHLIBDIR=	yes

.PATH:	${.CURDIR}/../../dist/common/avl

CPPFLAGS+=-Wall -Wno-unknown-pragmas

LIB=	avl
SRCS=	avl.c

.include "../../Makefile.zfs"
.include <bsd.lib.mk>
