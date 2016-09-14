# $NetBSD: Makefile,v 1.1.2.2 2016/09/14 03:04:18 pgoyette Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_stf
IOCONF=		stf.ioconf
SRCS=		if_stf.c

CPPFLAGS+=	-DINET
CPPFLAGS+=	-DINET6

.include <bsd.kmodule.mk>
