# $NetBSD: Makefile,v 1.1.8.2 2000/11/20 20:20:40 bouyer Exp $

PRIMARY_PROG=	bootxx_cd9660
CPPFLAGS+=	-DLIBSA_SINGLE_FILESYSTEM=cd9660 \
		-DBOOTXX_FS_NAME='"ISO 9660"'

# XXX the following could probably be increased
PRIMARY_MAX_LOAD!=	expr 8192 - 512

.include "../Makefile.booters"
