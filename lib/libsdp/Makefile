# $NetBSD: Makefile,v 1.1 2006/06/19 15:44:36 gdamore Exp $
# $Id: Makefile,v 1.1 2006/06/19 15:44:36 gdamore Exp $
# $FreeBSD: src/lib/libsdp/Makefile,v 1.4 2005/07/22 17:19:03 kensmith Exp $

LIB=		sdp
MAN=		sdp.3

SRCS=		search.c service.c session.c util.c
INCS=		sdp.h
INCSDIR=	/usr/include

CPPFLAGS+=	-I${.CURDIR}

MLINKS+=	sdp.3 SDP_GET8.3
MLINKS+=	sdp.3 SDP_GET16.3
MLINKS+=	sdp.3 SDP_GET32.3
MLINKS+=	sdp.3 SDP_GET64.3
MLINKS+=	sdp.3 SDP_GET128.3
MLINKS+=	sdp.3 SDP_PUT8.3
MLINKS+=	sdp.3 SDP_PUT16.3
MLINKS+=	sdp.3 SDP_PUT32.3
MLINKS+=	sdp.3 SDP_PUT64.3
MLINKS+=	sdp.3 SDP_PUT128.3
MLINKS+=	sdp.3 sdp_open.3
MLINKS+=	sdp.3 sdp_open_local.3
MLINKS+=	sdp.3 sdp_close.3
MLINKS+=	sdp.3 sdp_error.3
MLINKS+=	sdp.3 sdp_search.3
MLINKS+=	sdp.3 sdp_attr2desc.3
MLINKS+=	sdp.3 sdp_uuid2desc.3
MLINKS+=	sdp.3 sdp_register_service.3
MLINKS+=	sdp.3 sdp_unregister_service.3
MLINKS+=	sdp.3 sdp_change_service.3

.include <bsd.lib.mk>
