#	$NetBSD: Makefile,v 1.1.2.2 2020/04/13 07:46:06 martin Exp $

.include <bsd.own.mk>

BINDIR=/usr/bin

PROG=	fido2-assert
SRCS+=\
assert_get.c \
assert_verify.c \
fido2-assert.c \
base64.c \
bio.c \
credman.c \
pin.c \
token.c \
util.c

SRCS+=\
explicit_bzero.c \
readpassphrase.c \
recallocarray.c

.include <bsd.prog.mk>
