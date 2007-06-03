#	$NetBSD: Makefile.inc,v 1.2.12.1 2007/06/03 17:25:57 wrstuden Exp $

# net sources
.PATH: ${.CURDIR}/resolv
CPPFLAGS+=-DCOMPAT__RES -DUSE_POLL

SRCS+=	__dn_comp.c __res_close.c __res_send.c h_errno.c \
	herror.c res_comp.c res_data.c res_debug.c \
	res_init.c res_mkquery.c res_query.c res_send.c \
	res_state.c mtctxres.c
# For COMPAT__RES
SRCS+=	res_compat.c
