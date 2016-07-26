# $NetBSD: Makefile,v 1.1.26.1 2016/07/26 03:24:13 pgoyette Exp $

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
