# $NetBSD: Makefile,v 1.1.4.2 2012/04/17 00:01:58 yamt Exp $
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
