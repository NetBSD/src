# $NetBSD: Makefile,v 1.6 2002/09/18 13:31:53 lukem Exp $

NOLINT=	# defined
NOPIC=	# defined
NOPROFILE=# defined

LIB    = dst
SRCS    = dst_support.c dst_api.c hmac_link.c md5_dgst.c base64.c prandom.c

DHCPSRCDIR=	dst

CFLAGS += -DMINIRES_LIB -DHMAC_MD5

# only needed during build
libinstall::

.include <bsd.lib.mk>
