# $NetBSD: Makefile,v 1.1.6.2 2009/05/13 17:18:13 jym Exp $

PRIMARY_PROG=	bootxx_ffsv2
CPPFLAGS+=	-DLIBSA_SINGLE_FILESYSTEM=ffsv2 \
		-DBOOTXX_FS_NAME='"FFSv2"'
# Use small daddr_t to avoid code bloat
CPPFLAGS+=	-D__daddr_t=int32_t

PRIMARY_MAX_LOAD!=	expr 8192 - 512

.include "../Makefile.booters"
