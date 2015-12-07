# $NetBSD: Makefile,v 1.1 2015/12/07 03:25:57 pgoyette Exp $

.include "../Makefile.inc"

.PATH: ${S}/dev

KMOD=		clockctl
IOCONF=		clockctl.ioconf
SRCS=		clockctl.c

CPPFLAGS+=	-DNTP -DCOMPAT_50

.include <bsd.kmodule.mk>
