# $NetBSD: Makefile,v 1.2 2003/06/24 09:38:36 tron Exp $

SUBDIR= bootxx_ufs bootxx_ffsv1 bootxx_ffsv2 bootxx_lfsv1 bootxx_lfsv2
SUBDIR+= bootxx_dosfs bootxx_ustarfs

.include <bsd.subdir.mk>

cleandir distclean: cleanlibdir

cleanlibdir:
	rm -rf lib
