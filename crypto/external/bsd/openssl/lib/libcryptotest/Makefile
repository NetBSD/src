# $NetBSD: Makefile,v 1.1 2018/09/23 13:33:09 christos Exp $

LIBISPRIVATE=	yes
LIB=		cryptotest

WARNS?=		5

.include <bsd.init.mk>

CRYPTODIST=     ${NETBSDSRCDIR}/crypto
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"
.PATH:	${OPENSSLSRC}/test/testutil
CPPFLAGS+= -I${OPENSSLSRC}/include

SRCS+=	tests.c format_output.c output_helpers.c basic_output.c stanza.c
SRCS+=	main.c tap_bio.c driver.c init.c test_cleanup.c cb.c

.include <bsd.lib.mk>
