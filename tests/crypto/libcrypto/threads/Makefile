# $NetBSD: Makefile,v 1.3 2018/02/08 21:59:11 christos Exp $

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

.include <bsd.init.mk>

.PATH:		${OPENSSLSRC}/apps

.if ${HAVE_OPENSSL} == 10
HELPER_SRCS=	mttest.c
.else
HELPER_SRCS=	threadstest.c
.endif
