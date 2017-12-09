#      $NetBSD: Makefile,v 1.1 2017/12/09 08:03:07 pgoyette Exp $

.include "../Makefile.inc"

.PATH: ${S}/netinet

KMOD=	ip_ecn
SRCS=	ip_ecn.c

CPPFLAGS+=	-DINET
CPPFLAGS+=	-DINET6

.include <bsd.kmodule.mk>
