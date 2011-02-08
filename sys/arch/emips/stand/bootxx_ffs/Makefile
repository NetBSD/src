# $NetBSD: Makefile,v 1.1.2.2 2011/02/08 18:05:09 bouyer Exp $

PRIMARY_PROG=	bootxx_ffs
CPPFLAGS+=	-DLIBSA_SINGLE_FILESYSTEM=ufs \
		-DBOOTXX_FS_NAME='"FFS"'

# Use small daddr_t to avoid code bloat
CPPFLAGS+=	-D__daddr_t=int32_t

PRIMARY_MAX_LOAD!=	expr 8192 + 8192

.include "../Makefile.booters"
