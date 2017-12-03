#	$NetBSD: Makefile,v 1.1.18.2 2017/12/03 11:38:59 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/net

KMOD=	slcompress
SRCS=	slcompress.c

CPPFLAGS+=	-DINET

.include <bsd.kmodule.mk>
