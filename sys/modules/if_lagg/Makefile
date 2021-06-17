# $NetBSD: Makefile,v 1.1.6.2 2021/06/17 04:46:34 thorpej Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net/lagg

KMOD=		if_lagg
IOCONF=		lagg.ioconf
SRCS= 		if_lagg.c \
		if_laggproto.c \
		if_lagg_lacp.c

CPPFLAGS+=	-DINET
CPPFLAGS+=	-DINET6
#CPPFLAGS+=	-DLAGG_DEBUG
#CPPFLAGS+=	-DLACP_DEBUG

WARNS=		5

.include <bsd.kmodule.mk>
