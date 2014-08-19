# $NetBSD: Makefile,v 1.1.20.1 2014/08/20 00:02:51 tls Exp $

PRIMARY_PROG=	bootxx_cd9660
CPPFLAGS+=	-DLIBSA_SINGLE_FILESYSTEM=cd9660 \
		-DBOOTXX_FS_NAME='"ISO 9660"'

# XXX the following could probably be increased
PRIMARY_MAX_LOAD=	8192

.include "../Makefile.booters"
