# $NetBSD: Makefile,v 1.1 2012/01/28 16:04:15 christos Exp $

LIBISPRIVATE=yes

.include "../Makefile.inc"

.PATH.c: ${DIST}/lib

LIB=tpm_utils
SRCS= \
tpm_log.c \
tpm_tspi.c \
tpm_utils.c    

.include <bsd.lib.mk>
