# $NetBSD: Makefile,v 1.2.2.2 2009/05/13 19:19:22 jym Exp $

HELPER_NAME=	threadstest
HELPER_DIR=	threads
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
