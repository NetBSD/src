# $NetBSD: Makefile,v 1.1 2025/07/17 14:26:09 christos Exp $

LIBISPRIVATE=	yes
LIB=		cryptotest

WARNS?=		5

.include <bsd.init.mk>

CRYPTODIST=     ${NETBSDSRCDIR}/crypto
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"
.PATH:	${OPENSSLSRC}/test/testutil ${OPENSSLSRC}/apps/lib
CPPFLAGS+= -I${OPENSSLSRC}
CPPFLAGS+= -I${OPENSSLSRC}/include
CPPFLAGS+= -I${OPENSSLSRC}/apps/include

SRCS+=\
apps_shims.c \
basic_output.c \
cb.c \
driver.c \
fake_random.c \
format_output.c \
load.c \
main.c \
options.c \
output.c \
provider.c \
random.c \
stanza.c \
test_cleanup.c \
test_options.c \
tests.c \
testutil_init.c

SRCS+= \
opt.c

COPTS.opt.c+=		-Wno-missing-field-initializers
COPTS.options.c+=	-Wno-format-nonliteral
COPTS.test_options.c+=	-Wno-missing-field-initializers

.include <bsd.lib.mk>
