#	$NetBSD: Makefile,v 1.1 2021/05/17 04:07:44 yamaguchi Exp $
#

.PATH:	${.CURDIR}/../../../../net/lagg

LIB=	rumpnet_lagg
COMMENT=Layer 2 trunking pseudo interface

IOCONF=	LAGG.ioconf
SRCS=	if_lagg.c
SRCS+=	if_laggproto.c
SRCS+=	if_lagg_lacp.c

SRCS+=	lagg_component.c

.ifdef RUMP_DEBUG
CPPFLAGS.if_lagg.c+=		-DLAGG_DEBUG
CPPFLAGS.if_lagg_lacp.c+=	-DLACP_DEBUG
.endif

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
