# $NetBSD: Makefile,v 1.1.10.2 2014/05/22 15:44:36 yamt Exp $

LIBISPRIVATE=	yes

LIB    = dst
SRCS    = dst_support.c dst_api.c hmac_link.c md5_dgst.c base64.c prandom.c

DHCPSRCDIR=	dst

CFLAGS += -DMINIRES_LIB -DHMAC_MD5

.include <bsd.lib.mk>
