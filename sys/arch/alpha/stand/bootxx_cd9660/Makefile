# $NetBSD: Makefile,v 1.3 2000/09/26 05:13:37 simonb Exp $

PROG = bootxx_cd9660
CPPFLAGS += -DLIBSA_SINGLE_FILESYSTEM=cd9660 \
	    -DBOOTXX_FS_NAME='"ISO 9660"' \
	    -DBOOTXX_RAID1_SUPPORT \
	    -DNO_GETCHAR

# not yet desired: -DBOOTXX_FS_TYPE=FS_ISO9660

# XXX the following could probably be increased
PRIMARY_MAX_LOAD!=	expr 8192 - 512

.include "../Makefile.bootxx"
