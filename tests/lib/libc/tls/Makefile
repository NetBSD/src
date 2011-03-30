# $NetBSD: Makefile,v 1.3 2011/03/30 11:43:15 he Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/tls
.if (${MKPIC} == "yes")
SUBDIR+=	dso
.endif

TESTS_C+=	t_tls_static
.if (${MKPIC} == "yes")
TESTS_C+=	t_tls_dynamic t_tls_dlopen
.endif

DSODIR!=	cd ${.CURDIR}/../tls_dso && ${PRINTOBJDIR}

SRCS.t_tls_static=	t_tls_static.c t_tls_static_helper.c
LDADD.t_tls_static+=	-lpthread -static
LDADD.t_tls_dynamic+=	-lpthread \
			-Wl,-rpath,${TESTSDIR} -L${DSODIR} -lh_tls_dynamic
LDADD.t_tls_dlopen+=	-lpthread -Wl,-rpath,${TESTSDIR} -Wl,-export-dynamic

.include <bsd.test.mk>
