#	$NetBSD: Makefile,v 1.5.16.1 2019/09/27 09:24:30 martin Exp $

.include <bsd.init.mk>

USE_SHLIBDIR=	yes

.PATH:	${.CURDIR}/../../dist/common/avl

CPPFLAGS+=-Wall -Wno-unknown-pragmas

LIB=	avl
SRCS=	avl.c

.include "../../Makefile.zfs"
.include <bsd.lib.mk>
