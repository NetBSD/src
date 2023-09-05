# $NetBSD: Makefile,v 1.3.24.1 2023/09/05 17:18:22 martin Exp $
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
