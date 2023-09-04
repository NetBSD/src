# $NetBSD: Makefile,v 1.4 2023/09/04 18:12:46 riastradh Exp $
.include "../Makefile.inc"

.PATH.c: ${DIST}/lib

LIB=tpm_unseal
SRCS= \
tpm_unseal.c

SHLIB_MAJOR=2
SHLIB_MINOR=0

LIBDPLIBS+=tspi ${.CURDIR}/../../../trousers/lib/libtspi
COPTS.tpm_unseal.c+=-Wno-stack-protector

.include <bsd.lib.mk>
