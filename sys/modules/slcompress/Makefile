#	$NetBSD: Makefile,v 1.3 2020/10/18 15:02:12 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/net

KMOD=	slcompress
SRCS=	slcompress.c

CPPFLAGS+=	-DINET

CWARNFLAGS.slcompress.c+=   ${GCC_NO_ADDR_OF_PACKED_MEMBER}


WARNS=	3

.include <bsd.kmodule.mk>
