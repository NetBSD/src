# $NetBSD: Makefile,v 1.4 2018/02/08 23:52:26 christos Exp $

HELPER_NAME=	threadstest
HELPER_DIR=	threads

CPPFLAGS+=	-DPTHREADS
CFLAGS+=	-pthread

DPADD+=		${LIBPTHREAD} ${LIBSSL}
LDADD+=		-lpthread -lssl

FILESDIR=	${TESTSDIR}
FILES=			server.pem
FILESNAME_server.pem=	d_server.pem
FILES+=			client.pem
FILESNAME_client.pem=	d_client.pem

.PATH:		${OPENSSLSRC}/apps

HELPER_SRCS=	${${HAVE_OPENSSL} == 10 :? mttest.c : threadstest.c}

.include <bsd.init.mk>
