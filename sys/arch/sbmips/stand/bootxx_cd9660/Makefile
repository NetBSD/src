# $NetBSD: Makefile,v 1.1 2002/11/09 06:20:38 cgd Exp $

PROG = bootxx_cd9660
CPPFLAGS += -DLIBSA_SINGLE_FILESYSTEM=cd9660 \
	    -DBOOTXX_FS_NAME='"ISO 9660"' \
	    -DNO_GETCHAR

# not yet desired: -DBOOTXX_FS_TYPE=FS_ISO9660

# XXX the following could probably be increased
PRIMARY_MAX_LOAD!=	expr 8192 - 512

.include "../Makefile.bootxx"
