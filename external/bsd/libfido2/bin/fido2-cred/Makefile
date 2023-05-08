#	$NetBSD: Makefile,v 1.3 2023/05/08 23:45:52 christos Exp $

.include <bsd.own.mk>

BINDIR=/usr/bin

PROG=	fido2-cred
SRCS+=\
config.c \
cred_make.c \
cred_verify.c \
base64.c \
bio.c \
credman.c \
fido2-cred.c \
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
