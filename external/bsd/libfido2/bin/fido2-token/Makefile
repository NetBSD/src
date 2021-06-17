#	$NetBSD: Makefile,v 1.2 2021/06/17 01:15:44 christos Exp $

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

.include <bsd.prog.mk>
