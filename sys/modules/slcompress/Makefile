#	$NetBSD: Makefile,v 1.5 2023/05/29 20:31:10 lukem Exp $

.include "../Makefile.inc"

.PATH:	${S}/net

KMOD=	slcompress
SRCS=	slcompress.c

CPPFLAGS+=	-DINET

COPTS.slcompress.c+=	${CC_NO_ADDR_OF_PACKED_MEMBER}


WARNS=	3

.include <bsd.kmodule.mk>
