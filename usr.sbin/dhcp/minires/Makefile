# $NetBSD: Makefile,v 1.1.4.2 2001/08/16 18:53:35 tv Exp $

LIB    = minires
SRCS   = res_mkupdate.c res_init.c res_update.c res_send.c res_comp.c \
	 res_sendsigned.c res_findzonecut.c res_query.c res_mkquery.c \
	 ns_date.c ns_parse.c ns_sign.c ns_name.c ns_samedomain.c \
	 ns_verify.c

DHCPSRCDIR=	minires

MKLINT=	no
MKPIC=	no
MKPROFILE=no

CFLAGS += -DMINIRES_LIB

# only needed during build
libinstall::

.include <bsd.lib.mk>
