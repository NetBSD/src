# $NetBSD: Makefile,v 1.1.8.2 2000/11/20 20:20:41 bouyer Exp $

PRIMARY_PROG=	bootxx_ffs
CPPFLAGS+=	-DLIBSA_SINGLE_FILESYSTEM=ufs \
		-DBOOTXX_FS_NAME='"FFS"'

PRIMARY_MAX_LOAD!=	expr 8192 - 512

.include "../Makefile.booters"
