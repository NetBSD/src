#	$NetBSD: Makefile,v 1.3 2023/05/08 23:45:52 christos Exp $

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
