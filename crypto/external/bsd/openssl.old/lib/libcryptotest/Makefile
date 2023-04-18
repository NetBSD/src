# $NetBSD: Makefile,v 1.1.1.1 2023/04/18 14:19:16 christos Exp $

LIBISPRIVATE=	yes
LIB=		cryptotest

WARNS?=		5

.include <bsd.init.mk>

CRYPTODIST=     ${NETBSDSRCDIR}/crypto
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"
.PATH:	${OPENSSLSRC}/test/testutil
CPPFLAGS+= -I${OPENSSLSRC}/include

SRCS+=	tests.c format_output.c output_helpers.c basic_output.c stanza.c
SRCS+=	main.c tap_bio.c driver.c test_cleanup.c cb.c random.c  testutil_init.c

.include <bsd.lib.mk>
