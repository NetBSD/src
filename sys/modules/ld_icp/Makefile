#	$NetBSD: Makefile,v 1.1.4.2 2016/11/04 14:49:19 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic

KMOD=	ld_icp
IOCONF=	ld_icp.ioconf

SRCS+=	ld_icp.c

CPPFLAGS+=	-DNBIO=1

.include <bsd.kmodule.mk>
