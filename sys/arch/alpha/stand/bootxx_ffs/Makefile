# $NetBSD: Makefile,v 1.2.2.2 1999/04/05 03:37:30 cgd Exp $

PROG = bootxx_ffs
CPPFLAGS += -DLIBSA_SINGLE_FILESYSTEM=ufs \
	    -DBOOTXX_FS_NAME='"FFS"' \
	    -DNO_GETCHAR

# not yet desired: -DBOOTXX_FS_TYPE=FS_BSDFFS

PRIMARY_MAX_LOAD!=	expr 8192 - 512

.include "../Makefile.bootxx"
