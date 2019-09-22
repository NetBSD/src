#	$NetBSD: Makefile,v 1.6 2019/09/22 18:45:19 brad Exp $

.include <bsd.init.mk>

USE_SHLIBDIR=	yes

.PATH:	${.CURDIR}/../../dist/common/avl

CPPFLAGS+=-Wall -Wno-unknown-pragmas

LIB=	avl
SRCS=	avl.c

.include "../../Makefile.zfs"
.include <bsd.lib.mk>
