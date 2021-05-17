# $NetBSD: Makefile,v 1.1 2021/05/17 04:07:43 yamaguchi Exp $

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
