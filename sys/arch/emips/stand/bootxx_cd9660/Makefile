# $NetBSD: Makefile,v 1.1 2011/01/26 01:18:53 pooka Exp $

PRIMARY_PROG=	bootxx_cd9660
CPPFLAGS+=	-DLIBSA_SINGLE_FILESYSTEM=cd9660 \
		-DBOOTXX_FS_NAME='"ISO 9660"'

# XXX the following could probably be increased
PRIMARY_MAX_LOAD!=	expr 8192

.include "../Makefile.booters"
