#	$NetBSD: Makefile,v 1.2.2.1 2023/08/11 13:42:55 martin Exp $

.include <bsd.own.mk>

BINDIR=/usr/bin

PROG=	fido2-assert
SRCS+=\
config.c \
assert_get.c \
assert_verify.c \
fido2-assert.c \
largeblob.c \
base64.c \
bio.c \
credman.c \
pin.c \
token.c \
util.c

SRCS+=\
explicit_bzero.c \
freezero.c \
readpassphrase.c \
recallocarray.c

COPTS.assert_verify.c+=-Wno-error=deprecated-declarations
COPTS.util.c+=-Wno-error=deprecated-declarations

.include <bsd.prog.mk>
