# $NetBSD: Makefile,v 1.2 2005/12/11 12:16:20 christos Exp $

PROG = bootxx_ffsv2
CPPFLAGS += -DLIBSA_SINGLE_FILESYSTEM=ffsv2 \
	    -DBOOTXX_FS_NAME='"FFSv2"' \
	    -DBOOTXX_RAID1_SUPPORT \
	    -DNO_GETCHAR
# Use small daddr_t to avoid code bloat
CPPFLAGS+=	-D__daddr_t=int32_t

# not yet desired: -DBOOTXX_FS_TYPE=FS_BSDFFS

PRIMARY_MAX_LOAD!=	expr 8192 - 512

.include "../Makefile.bootxx"
