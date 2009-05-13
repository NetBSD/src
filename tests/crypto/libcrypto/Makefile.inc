# $NetBSD: Makefile.inc,v 1.1.2.2 2009/05/13 19:19:18 jym Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/crypto/libcrypto

.if defined(HELPER_NAME)
HELPER_SRCS?=	${HELPER_NAME}.c

PROG=		h_${HELPER_NAME}
SRCS=		${HELPER_SRCS}
MAN=		# empty
BINDIR=		${TESTSDIR}

NOATFFILE=	yes

DPADD+=		${LIBCRYPTO} ${LIBCRYPT}
LDADD+=		-lcrypto -lcrypt

CPPFLAGS+=	-DOPENSSL_FIPS
CPPFLAGS+=	-I${OPENSSLSRC}
CRYPTODIST=	${NETBSDSRCDIR}/crypto/dist
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"
.PATH:	${OPENSSLSRC}/crypto/${HELPER_DIR}

.include <bsd.test.mk>
.include <bsd.prog.mk>
.endif
