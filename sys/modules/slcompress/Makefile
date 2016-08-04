#	$NetBSD: Makefile,v 1.1 2016/08/04 23:53:47 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/net

KMOD=	slcompress
SRCS=	slcompress.c

CPPFLAGS+=	-DINET

.include <bsd.kmodule.mk>
