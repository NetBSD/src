# $NetBSD: Makefile,v 1.1.20.2 2018/09/24 00:02:50 pgoyette Exp $

.include "../Makefile.inc"

.PATH: ${S}/dev

CPPFLAGS+=	-DNTP

KMOD=		clockctl
IOCONF=		clockctl.ioconf
SRCS=		clockctl.c

.include <bsd.kmodule.mk>
