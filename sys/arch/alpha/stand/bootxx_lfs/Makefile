# $NetBSD: Makefile,v 1.3 2003/02/01 23:58:48 simonb Exp $

PROG = bootxx_lfs
CPPFLAGS += -DLIBSA_SINGLE_FILESYSTEM=lfs \
	    -DBOOTXX_FS_NAME='"LFS"' \
	    -DBOOTXX_RAID1_SUPPORT \
	    -DNO_GETCHAR
# Use small daddr_t to avoid code bloat
CPPFLAGS+=	-D__daddr_t=int32_t

# not yet desired: -DBOOTXX_FS_TYPE=FS_BSDLFS

PRIMARY_MAX_LOAD!=	expr 8192 - 512

.include "../Makefile.bootxx"
