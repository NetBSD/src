# $NetBSD: Makefile,v 1.1 2012/01/28 16:04:15 christos Exp $
.include "../Makefile.inc"

.PATH.c: ${DIST}/lib

LIB=tpm_unseal
SRCS= \
tpm_unseal.c

SHLIB_MAJOR=1
SHLIB_MINOR=0

LDADD+=-ltspi
DPADD+=${LIBTSPI}

.include <bsd.lib.mk>
