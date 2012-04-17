# $NetBSD: Makefile,v 1.1.4.2 2012/04/17 00:01:58 yamt Exp $

LIBISPRIVATE=yes

.include "../Makefile.inc"

.PATH.c: ${DIST}/lib

LIB=tpm_utils
SRCS= \
tpm_log.c \
tpm_tspi.c \
tpm_utils.c    

.include <bsd.lib.mk>
