#	$NetBSD: Makefile,v 1.1.2.2 2000/11/20 20:14:13 bouyer Exp $

PRIMARY_PROG=	bootxx_ffs
CPPFLAGS+=	-DLIBSA_SINGLE_FILESYSTEM=ufs \
		-DBOOTXX_FS_NAME='"FFS"'

PRIMARY_MAX_LOAD!=	expr 8192 - 1024

.include "../Makefile.booters"
