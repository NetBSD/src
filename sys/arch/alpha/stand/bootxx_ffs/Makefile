# $NetBSD: Makefile,v 1.2.18.1 2000/09/26 15:50:14 simonb Exp $

PROG = bootxx_ffs
CPPFLAGS += -DLIBSA_SINGLE_FILESYSTEM=ufs \
	    -DBOOTXX_FS_NAME='"FFS"' \
	    -DBOOTXX_RAID1_SUPPORT \
	    -DNO_GETCHAR

# not yet desired: -DBOOTXX_FS_TYPE=FS_BSDFFS

PRIMARY_MAX_LOAD!=	expr 8192 - 512

.include "../Makefile.bootxx"
