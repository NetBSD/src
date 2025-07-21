# $NetBSD: Makefile.inc,v 1.13 2025/07/21 22:17:56 christos Exp $

.include <bsd.own.mk>
.include "../Makefile.inc"
TESTSDIR=	${TESTSBASE}/crypto/libcrypto
ATFFILE=	no

.if defined(HELPER_NAME)
HELPER_SRCS?=	${HELPER_NAME}.c

PROG=		h_${HELPER_NAME}
SRCS=		${HELPER_SRCS}
MAN=		# empty
BINDIR=		${TESTSDIR}

CWARNFLAGS.clang+=	-Wno-format
CWARNFLAGS+=-Wno-error=deprecated-declarations

CPPFLAGS+=	-DOPENSSL_FIPS
CPPFLAGS+=	-I${OPENSSLSRC} -I${OPENSSLSRC}/crypto
CPPFLAGS+=	-I${OPENSSLSRC}/include -I${OPENSSLSRC}/../include
CPPFLAGS+=	-I${OPENSSLSRC}/crypto/include
CPPFLAGS+=	-I${OPENSSLSRC}/apps/include
CRYPTODIST=	${NETBSDSRCDIR}/crypto
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"
.if ${HAVE_OPENSSL} != 10
.PATH:	${OPENSSLSRC}/test ${OPENSSLSRC}/test/testutil
.else
.PATH: ${OPENSSLSRC}/crypto/${HELPER_DIR}
.endif
PROGDPLIBS+=	cryptotest ${OPENSSLSRC}/../lib/libcryptotest
.if ${HELPER_NAME} != "threadstest"
PROGDPLIBS+=	crypto ${OPENSSLSRC}/../lib/libcrypto
.endif
DPADD_AFTER+=		${LIBCRYPT} ${LIBPTHREAD}
LDADD_AFTER+=		-lcrypt -lpthread


.include <bsd.test.mk>
.include <bsd.prog.mk>
.endif
