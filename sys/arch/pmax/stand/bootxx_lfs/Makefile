# $NetBSD: Makefile,v 1.1.2.1 1999/12/27 18:33:31 wrstuden Exp $

PRIMARY_PROG=	bootxx_lfs
CPPFLAGS+=	-DLIBSA_SINGLE_FILESYSTEM=lfs \
		-DBOOTXX_FS_NAME='"LFS"'

PRIMARY_MAX_LOAD!=	expr 8192 - 512

.include "../Makefile.booters"
