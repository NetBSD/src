# $NetBSD: Makefile,v 1.5 2001/12/12 01:48:50 tv Exp $

LIB    = dst
SRCS    = dst_support.c dst_api.c hmac_link.c md5_dgst.c base64.c prandom.c

DHCPSRCDIR=	dst

NOLINT=	# defined
NOPIC=	# defined
NOPROFILE=# defined

CFLAGS += -DMINIRES_LIB -DHMAC_MD5

# only needed during build
libinstall::

.include <bsd.lib.mk>
