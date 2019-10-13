#	$NetBSD: Makefile.inc,v 1.8 2019/10/13 07:28:10 mrg Exp $

# net sources
.PATH: ${.CURDIR}/resolv
CPPFLAGS+=-DCOMPAT__RES -DUSE_POLL

SRCS+=	h_errno.c herror.c res_comp.c res_data.c res_debug.c \
	res_init.c res_mkquery.c res_query.c res_send.c \
	res_state.c mtctxres.c
# For COMPAT__RES
SRCS+=	res_compat.c

COPTS.res_query.c+=	${GCC_NO_STRINGOP_OVERFLOW}
