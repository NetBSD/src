# $NetBSD: Makefile,v 1.9 2025/07/18 00:51:36 christos Exp $

.include <bsd.own.mk>
HELPER_NAME=	threadstest
HELPER_DIR=	threads

CPPFLAGS+=	-DPTHREADS
CFLAGS+=	-pthread

COPTS.threadstest.c+=	-Wno-missing-field-initializers

DPADD+=		${LIBPTHREAD} ${LIBSSL}
LDADD+=		-lpthread -lssl


FILESDIR=	${TESTSDIR}
FILES=			server.pem
FILESNAME_server.pem=	d_server.pem
FILES+=			client.pem
FILESNAME_client.pem=	d_client.pem
.if ${HAVE_OPENSSL} > 11
FILES+=rsakey.pem
FILES+=default.cnf
.endif

HELPER_SRCS=	${${HAVE_OPENSSL} == 10 :? mttest.c : threadstest.c}

.include <bsd.init.mk>

.PATH:		${OPENSSLSRC}/apps
.PATH:		${OPENSSLSRC}/test/recipes/90-test_threads_data
LIBCRYPTO!= cd ${OPENSSLSRC}/../lib/libcrypto && ${PRINTOBJDIR}
LDADD_AFTER+=  ${LIBCRYPTO}/libcrypto.a
DPADD_AFTER+=          ${LIBCRYPT}
