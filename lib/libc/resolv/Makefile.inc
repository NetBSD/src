#	$NetBSD: Makefile.inc,v 1.1 2004/05/20 18:00:55 christos Exp $

# net sources
.PATH: ${.CURDIR}/resolv

SRCS+=	__dn_comp.c __res_close.c __res_send.c h_errno.c \
	herror.c res_comp.c res_data.c res_debug.c \
	res_init.c res_mkquery.c res_query.c res_send.c \
	res_state.c
