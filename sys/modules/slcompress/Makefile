#	$NetBSD: Makefile,v 1.2 2019/02/17 04:05:57 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/net

KMOD=	slcompress
SRCS=	slcompress.c

CPPFLAGS+=	-DINET

WARNS=	3

.include <bsd.kmodule.mk>
