# $NetBSD: Makefile,v 1.1 1999/04/02 03:38:20 cgd Exp $

PROG = bootxx_ffs
CPPFLAGS += -DLIBSA_SINGLE_FILESYSTEM=ufs \
	    -DBOOTXX_FS_NAME='"FFS"' \
	    -DNO_GETCHAR

# not yet desired: -DBOOTXX_FS_TYPE=FS_BSDFFS

PRIMARY_MAX_LOAD!=	expr 8192 - 512

# this and the following lines are bugs, which should be fixed.
install:

.include "../Makefile.bootxx"
