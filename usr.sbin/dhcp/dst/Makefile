# $NetBSD: Makefile,v 1.2.2.3 2001/08/16 18:53:34 tv Exp $

LIB    = dst
SRCS    = dst_support.c dst_api.c hmac_link.c md5_dgst.c base64.c prandom.c

DHCPSRCDIR=	dst

MKLINT=	no
MKPIC=	no
MKPROFILE=no

CFLAGS += -DMINIRES_LIB -DHMAC_MD5

# only needed during build
libinstall::

.include <bsd.lib.mk>
