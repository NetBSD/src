# $NetBSD: Makefile,v 1.2.122.1 2009/05/13 17:18:13 jym Exp $

PRIMARY_PROG=	bootxx_ffs
CPPFLAGS+=	-DLIBSA_SINGLE_FILESYSTEM=ffsv1 \
		-DBOOTXX_FS_NAME='"FFSv1"'
# Use small daddr_t to avoid code bloat
CPPFLAGS+=	-D__daddr_t=int32_t

PRIMARY_MAX_LOAD!=	expr 8192 - 512

.include "../Makefile.booters"
