# $NetBSD: Makefile,v 1.7 2023/06/03 09:09:15 lukem Exp $

TESTSDIR=	${TESTSBASE}/lib/libevent

TESTS_SH=	t_event

.include <bsd.own.mk>
.include <../../../external/bsd/libevent/Makefile.inc>

.PATH:		${LIBEVENT_DIST}/test

BINDIR=		${TESTSDIR}

PROG=		h_event
MAN=		# defined
SRCS=		regress.c regress.gen.c regress_dns.c regress_http.c \
		regress_rpc.c regress_main.c tinytest.c regress_testutils.c \
		regress_minheap.c regress_util.c regress_thread.c \
		regress_buffer.c regress_bufferevent.c regress_listener.c \
		regress_zlib.c regress_et.c regress_finalize.c

SRCS+=		regress_ssl.c
DPADD+=		${LIBEVENT_OPENSSL} ${LIBSSL} ${LIBCRYPTO}
LDADD+=		-levent_openssl -lssl -lcrypto
CPPFLAGS+=	-DLIBEVENT_CRYPTO
DPADD+=		${LIBEVENT} ${LIBEVENT_PTHREADS} ${LIBZ} ${LIBPTHREAD}
LDADD+=		-levent_pthreads -levent -lz -lpthread

COPTS.regress_rpc.c+=	${CC_WNO_CAST_FUNCTION_TYPE}
COPTS.regress_dns.c+=	${CC_WNO_MAYBE_UNINITIALIZED}

.include <bsd.test.mk>
