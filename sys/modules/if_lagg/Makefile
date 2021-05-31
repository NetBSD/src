# $NetBSD: Makefile,v 1.1.2.2 2021/05/31 22:15:21 cjep Exp $

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
