#	$NetBSD: Makefile,v 1.2 2003/02/02 00:08:55 simonb Exp $

PRIMARY_PROG=	bootxx_ffs
CPPFLAGS+=	-DLIBSA_SINGLE_FILESYSTEM=ufs \
		-DBOOTXX_FS_NAME='"FFS"'
# Use small daddr_t to avoid code bloat
CPPFLAGS+=	-D__daddr_t=int32_t

PRIMARY_MAX_LOAD!=	expr 8192 - 1024

.include "../Makefile.booters"
