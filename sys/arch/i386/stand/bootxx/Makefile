# $NetBSD: Makefile,v 1.3 2003/10/08 04:25:45 lukem Exp $

SUBDIR=	bootxx_ffsv1 bootxx_ffsv2 bootxx_lfsv1 bootxx_lfsv2
SUBDIR+=bootxx_msdos bootxx_ustarfs

.include <bsd.subdir.mk>

cleandir distclean: cleanlibdir

cleanlibdir:
	rm -rf lib
