#	$NetBSD: Makefile,v 1.1.4.2 2016/10/05 20:56:07 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/net

KMOD=	slcompress
SRCS=	slcompress.c

CPPFLAGS+=	-DINET

.include <bsd.kmodule.mk>
