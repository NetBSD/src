# $NetBSD: Makefile,v 1.7.24.1 2008/05/18 12:36:14 yamt Exp $

LIBISPRIVATE=	yes

LIB    = dst
SRCS    = dst_support.c dst_api.c hmac_link.c md5_dgst.c base64.c prandom.c

DHCPSRCDIR=	dst

CFLAGS += -DMINIRES_LIB -DHMAC_MD5

.include <bsd.lib.mk>
