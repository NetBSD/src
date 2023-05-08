#	$NetBSD: Makefile,v 1.3 2023/05/08 23:45:52 christos Exp $

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
