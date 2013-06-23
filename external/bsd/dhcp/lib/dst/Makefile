# $NetBSD: Makefile,v 1.1.4.2 2013/06/23 06:26:31 tls Exp $

LIBISPRIVATE=	yes

LIB    = dst
SRCS    = dst_support.c dst_api.c hmac_link.c md5_dgst.c base64.c prandom.c

DHCPSRCDIR=	dst

CFLAGS += -DMINIRES_LIB -DHMAC_MD5

.include <bsd.lib.mk>
