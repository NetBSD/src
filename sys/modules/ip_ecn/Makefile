#      $NetBSD: Makefile,v 1.1.2.2 2017/12/21 19:14:41 snj Exp $

.include "../Makefile.inc"

.PATH: ${S}/netinet

KMOD=	ip_ecn
SRCS=	ip_ecn.c

CPPFLAGS+=	-DINET
CPPFLAGS+=	-DINET6

.include <bsd.kmodule.mk>
