# $NetBSD: Makefile,v 1.2 2013/10/04 15:02:14 christos Exp $

PRIMARY_PROG=	bootxx_cd9660
CPPFLAGS+=	-DLIBSA_SINGLE_FILESYSTEM=cd9660 \
		-DBOOTXX_FS_NAME='"ISO 9660"'

# XXX the following could probably be increased
PRIMARY_MAX_LOAD=	8192

.include "../Makefile.booters"
