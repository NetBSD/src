# $NetBSD: Makefile,v 1.6 2001/12/12 01:48:50 tv Exp $

LIB    = minires
SRCS   = res_mkupdate.c res_init.c res_update.c res_send.c res_comp.c \
	 res_sendsigned.c res_findzonecut.c res_query.c res_mkquery.c \
	 ns_date.c ns_parse.c ns_sign.c ns_name.c ns_samedomain.c \
	 ns_verify.c

DHCPSRCDIR=	minires

NOLINT=	# defined
NOPIC=	# defined
NOPROFILE=# defined

CFLAGS += -DMINIRES_LIB

# only needed during build
libinstall::

.include <bsd.lib.mk>
