# $NetBSD: Makefile,v 1.1.4.2 2011/03/05 20:50:00 rmind Exp $

PRIMARY_PROG=	bootxx_cd9660
CPPFLAGS+=	-DLIBSA_SINGLE_FILESYSTEM=cd9660 \
		-DBOOTXX_FS_NAME='"ISO 9660"'

# XXX the following could probably be increased
PRIMARY_MAX_LOAD!=	expr 8192

.include "../Makefile.booters"
