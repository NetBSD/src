# $NetBSD: Makefile.inc,v 1.1.1.1 1998/10/05 18:01:58 tron Exp $

.PATH: ${.CURDIR}/resolv

SRCS+=	herror.c res_debug.c res_data.c res_comp.c res_init.c \
	res_mkquery.c res_query.c res_send.c res_mkupdate.c \
	res_update.c
