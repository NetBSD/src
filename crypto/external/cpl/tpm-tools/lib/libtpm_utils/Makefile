# $NetBSD: Makefile,v 1.2 2016/07/25 06:20:26 christos Exp $

LIBISPRIVATE=yes

.include "../Makefile.inc"

.PATH.c: ${DIST}/lib

LIB=tpm_utils
SRCS= \
tpm_log.c \
tpm_tspi.c \
tpm_utils.c    

COPTS.tpm_utils.c+=-Wno-stack-protector

.include <bsd.lib.mk>
