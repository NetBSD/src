# $NetBSD: Makefile,v 1.1.6.1 2020/01/27 07:21:48 martin Exp $

LIBISPRIVATE=	yes
LIB=		cryptotest

WARNS?=		5

.include <bsd.init.mk>

CRYPTODIST=     ${NETBSDSRCDIR}/crypto
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"
.PATH:	${OPENSSLSRC}/test/testutil
CPPFLAGS+= -I${OPENSSLSRC}/include

SRCS+=	tests.c format_output.c output_helpers.c basic_output.c stanza.c
SRCS+=	main.c tap_bio.c driver.c init.c test_cleanup.c cb.c random.c

.include <bsd.lib.mk>
