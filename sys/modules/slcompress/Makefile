#	$NetBSD: Makefile,v 1.4 2020/10/18 19:56:42 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/net

KMOD=	slcompress
SRCS=	slcompress.c

CPPFLAGS+=	-DINET

COPTS.slcompress.c+=   ${NO_ADDR_OF_PACKED_MEMBER}


WARNS=	3

.include <bsd.kmodule.mk>
