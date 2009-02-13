# $NetBSD: Makefile,v 1.1 2009/02/13 22:01:49 jmmv Exp $

HELPER_NAME=	threadstest
HELPER_DIR=	md5
HELPER_SRCS=	mttest.c

CPPFLAGS+=	-DPTHREADS
CFLAGS+=	-pthread

DPADD+=		${LIBPTHREAD} ${LIBSSL}
LDADD+=		-lpthread -lssl

FILESDIR=	${TESTSDIR}
FILES=			server.pem
FILESNAME_server.pem=	d_server.pem
FILES+=			client.pem
FILESNAME_client.pem=	d_client.pem

.include <bsd.init.mk>

.PATH:		${OPENSSLSRC}/apps
