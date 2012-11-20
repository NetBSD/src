# $NetBSD: Makefile,v 1.1.8.1 2012/11/20 02:57:29 tls Exp $
.include "../Makefile.inc"

.PATH.c: ${DIST}/lib

LIB=tpm_unseal
SRCS= \
tpm_unseal.c

SHLIB_MAJOR=1
SHLIB_MINOR=0

LIBDPLIBS+=tspi ${.CURDIR}/../../../trousers/lib/libtspi

.include <bsd.lib.mk>
