# $NetBSD: Makefile,v 1.1.2.2 2011/02/08 18:05:09 bouyer Exp $

PRIMARY_PROG=	bootxx_cd9660
CPPFLAGS+=	-DLIBSA_SINGLE_FILESYSTEM=cd9660 \
		-DBOOTXX_FS_NAME='"ISO 9660"'

# XXX the following could probably be increased
PRIMARY_MAX_LOAD!=	expr 8192

.include "../Makefile.booters"
