# $NetBSD: Makefile,v 1.8 2004/05/23 02:24:52 lukem Exp $

MKPRIVATELIB=	yes

LIB    = minires
SRCS   = res_mkupdate.c res_init.c res_update.c res_send.c res_comp.c \
	 res_sendsigned.c res_findzonecut.c res_query.c res_mkquery.c \
	 ns_date.c ns_parse.c ns_sign.c ns_name.c ns_samedomain.c \
	 ns_verify.c

DHCPSRCDIR=	minires

CFLAGS += -DMINIRES_LIB

.include <bsd.lib.mk>
