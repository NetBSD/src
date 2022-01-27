# $NetBSD: Makefile,v 1.2 2022/01/27 14:18:40 jakllsch Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_wg
IOCONF=		wg.ioconf
SRCS=		if_wg.c

CPPFLAGS+=	-DINET
CPPFLAGS+=	-DINET6
#CPPFLAGS+=	-DALTQ

WARNS=		3

.include <bsd.kmodule.mk>
