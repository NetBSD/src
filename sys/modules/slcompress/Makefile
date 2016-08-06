#	$NetBSD: Makefile,v 1.1.2.2 2016/08/06 00:19:10 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/net

KMOD=	slcompress
SRCS=	slcompress.c

CPPFLAGS+=	-DINET

.include <bsd.kmodule.mk>
