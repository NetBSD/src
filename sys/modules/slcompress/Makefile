#	$NetBSD: Makefile,v 1.1.22.1 2019/06/10 22:09:38 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/net

KMOD=	slcompress
SRCS=	slcompress.c

CPPFLAGS+=	-DINET

WARNS=	3

.include <bsd.kmodule.mk>
