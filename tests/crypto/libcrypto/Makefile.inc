# $NetBSD: Makefile.inc,v 1.5 2011/05/26 12:56:32 joerg Exp $

.include <bsd.own.mk>
# Not yet
# .include "../Makefile.inc"
TESTSDIR=	${TESTSBASE}/crypto/libcrypto
ATFFILE=	no

.if defined(HELPER_NAME)
HELPER_SRCS?=	${HELPER_NAME}.c

PROG=		h_${HELPER_NAME}
SRCS=		${HELPER_SRCS}
MAN=		# empty
BINDIR=		${TESTSDIR}

DPADD+=		${LIBCRYPTO} ${LIBCRYPT}
LDADD+=		-lcrypto -lcrypt

CWARNFLAGS.clang+=	-Wno-format

CPPFLAGS+=	-DOPENSSL_FIPS
CPPFLAGS+=	-I${OPENSSLSRC} -I${OPENSSLSRC}/crypto
CRYPTODIST=	${NETBSDSRCDIR}/crypto
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"
.PATH:	${OPENSSLSRC}/crypto/${HELPER_DIR}

.include <bsd.test.mk>
.include <bsd.prog.mk>
.endif
