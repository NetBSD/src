# $NetBSD: Makefile,v 1.1.1.1.6.2 2017/12/03 11:36:10 jdolecek Exp $

PROG = bootxx_ffs
CPPFLAGS += -DLIBSA_SINGLE_FILESYSTEM=ufs \
	    -DBOOTXX_FS_NAME='"FFS"' \
	    -DNO_GETCHAR
# Use small daddr_t to avoid code bloat
CPPFLAGS+=	-D__daddr_t=int32_t

# not yet desired: -DBOOTXX_FS_TYPE=FS_BSDFFS

PRIMARY_MAX_LOAD!=	expr 8192 - 512

.include "../Makefile.bootxx"
