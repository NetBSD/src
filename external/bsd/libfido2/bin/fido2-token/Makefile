#	$NetBSD: Makefile,v 1.2.2.1 2023/08/11 13:42:55 martin Exp $

.include <bsd.own.mk>

BINDIR=/usr/bin

PROG=	fido2-token
SRCS+=\
base64.c \
bio.c \
config.c \
credman.c \
fido2-token.c \
largeblob.c \
pin.c \
token.c \
util.c

SRCS+=\
explicit_bzero.c \
freezero.c \
readpassphrase.c \
recallocarray.c

COPTS.util.c+=-Wno-error=deprecated-declarations

.include <bsd.prog.mk>
