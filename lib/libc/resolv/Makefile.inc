#	$NetBSD: Makefile.inc,v 1.2.10.1 2007/05/17 00:43:51 jdc Exp $

# net sources
.PATH: ${.CURDIR}/resolv
CPPFLAGS+=-DCOMPAT__RES -DUSE_POLL

SRCS+=	__dn_comp.c __res_close.c __res_send.c h_errno.c \
	herror.c res_comp.c res_data.c res_debug.c \
	res_init.c res_mkquery.c res_query.c res_send.c \
	res_state.c mtctxres.c
# For COMPAT__RES
SRCS+=	res_compat.c
