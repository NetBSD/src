# $NetBSD: Makefile,v 1.1.4.3 2013/01/16 05:26:00 yamt Exp $
.include "../Makefile.inc"

.PATH.c: ${DIST}/lib

LIB=tpm_unseal
SRCS= \
tpm_unseal.c

SHLIB_MAJOR=1
SHLIB_MINOR=0

LIBDPLIBS+=tspi ${.CURDIR}/../../../trousers/lib/libtspi

.include <bsd.lib.mk>
