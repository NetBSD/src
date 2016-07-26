# $NetBSD: Makefile,v 1.2.14.1 2016/07/26 03:24:13 pgoyette Exp $
.include "../Makefile.inc"

.PATH.c: ${DIST}/lib

LIB=tpm_unseal
SRCS= \
tpm_unseal.c

SHLIB_MAJOR=1
SHLIB_MINOR=0

LIBDPLIBS+=tspi ${.CURDIR}/../../../trousers/lib/libtspi
COPTS.tpm_unseal.c+=-Wno-stack-protector

.include <bsd.lib.mk>
