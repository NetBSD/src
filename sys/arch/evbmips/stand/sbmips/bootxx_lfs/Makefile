# $NetBSD: Makefile,v 1.1.1.1 2017/07/24 08:56:29 mrg Exp $

PROG = bootxx_lfs
CPPFLAGS += -DLIBSA_SINGLE_FILESYSTEM=lfsv2 \
	    -DBOOTXX_FS_NAME='"LFS"' \
	    -DNO_GETCHAR
# Use small daddr_t to avoid code bloat
CPPFLAGS+=	-D__daddr_t=int32_t

# not yet desired: -DBOOTXX_FS_TYPE=FS_BSDLFS

PRIMARY_MAX_LOAD!=	expr 8192 - 512

.include "../Makefile.bootxx"
