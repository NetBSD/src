#	$NetBSD: Makefile,v 1.6 2023/06/03 08:52:58 lukem Exp $

.include "../Makefile.inc"

.PATH:	${S}/net

KMOD=	slcompress
SRCS=	slcompress.c

CPPFLAGS+=	-DINET

COPTS.slcompress.c+=	${CC_WNO_ADDRESS_OF_PACKED_MEMBER}


WARNS=	3

.include <bsd.kmodule.mk>
