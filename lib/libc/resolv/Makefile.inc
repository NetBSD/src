#	$NetBSD: Makefile.inc,v 1.2 2004/06/09 18:07:03 christos Exp $

# net sources
.PATH: ${.CURDIR}/resolv
CPPFLAGS+=-DCOMPAT__RES

SRCS+=	__dn_comp.c __res_close.c __res_send.c h_errno.c \
	herror.c res_comp.c res_data.c res_debug.c \
	res_init.c res_mkquery.c res_query.c res_send.c \
	res_state.c
# For COMPAT__RES
SRCS+=	res_compat.c
