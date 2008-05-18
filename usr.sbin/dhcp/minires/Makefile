# $NetBSD: Makefile,v 1.8.24.1 2008/05/18 12:36:14 yamt Exp $

LIBISPRIVATE=	yes

LIB    = minires
SRCS   = res_mkupdate.c res_init.c res_update.c res_send.c res_comp.c \
	 res_sendsigned.c res_findzonecut.c res_query.c res_mkquery.c \
	 ns_date.c ns_parse.c ns_sign.c ns_name.c ns_samedomain.c \
	 ns_verify.c

DHCPSRCDIR=	minires

CFLAGS += -DMINIRES_LIB

.include <bsd.lib.mk>
