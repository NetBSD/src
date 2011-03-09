# $NetBSD: Makefile,v 1.1 2011/03/09 23:10:07 joerg Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/tls
SUBDIR+=	dso

TESTS_C+=	t_tls_static t_tls_dynamic t_tls_dlopen
SRCS.t_tls_static=	t_tls_static.c t_tls_static_helper.c
LDADD.t_tls_static+=	-lpthread -static
LDADD.t_tls_dynamic+=	-lpthread \
			-Wl,-rpath,${TESTSDIR} -L../tls_dso -lh_tls_dynamic
LDADD.t_tls_dlopen+=	-lpthread -Wl,-rpath,${TESTSDIR} -Wl,-export-dynamic

.include <bsd.test.mk>
