#	$NetBSD: Makefile,v 1.1 2000/09/18 11:40:47 wdk Exp $

PRIMARY_PROG=	bootxx_ffs
CPPFLAGS+=	-DLIBSA_SINGLE_FILESYSTEM=ufs \
		-DBOOTXX_FS_NAME='"FFS"'

PRIMARY_MAX_LOAD!=	expr 8192 - 1024

.include "../Makefile.booters"
