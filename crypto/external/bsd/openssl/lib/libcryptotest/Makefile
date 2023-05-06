# $NetBSD: Makefile,v 1.4 2023/05/06 17:07:23 christos Exp $

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

.include <bsd.lib.mk>
