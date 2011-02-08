# $NetBSD: Makefile,v 1.1.2.2 2011/02/08 18:05:09 bouyer Exp $

PRIMARY_PROG=	bootxx_lfs
CPPFLAGS+=	-DLIBSA_SINGLE_FILESYSTEM=lfsv2 \
		-DBOOTXX_FS_NAME='"LFS"'
# Use small daddr_t to avoid code bloat
CPPFLAGS+=	-D__daddr_t=int32_t

PRIMARY_MAX_LOAD!=	expr 8192 + 512

.include "../Makefile.booters"
