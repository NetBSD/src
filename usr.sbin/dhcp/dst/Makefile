# $NetBSD: Makefile,v 1.3 2001/08/03 14:14:14 drochner Exp $

LIB    = dst
SRCS    = dst_support.c dst_api.c hmac_link.c md5_dgst.c base64.c prandom.c

DHCPSRCDIR=	dst

MKLINT=	no
MKPIC=	no
MKPROFILE=no

CFLAGS += -DMINIRES_LIB

# only needed during build
libinstall::

.include <bsd.lib.mk>
