# $NetBSD: Makefile,v 1.4 2003/10/08 18:51:50 dsl Exp $

SUBDIR=	bootxx_ffsv1 bootxx_ffsv2 bootxx_lfsv1 bootxx_lfsv2
SUBDIR+=bootxx_msdos bootxx_ustarfs

LIBOBJ= ${.OBJDIR}
.MAKEOVERRIDES+= LIBOBJ

.include <bsd.subdir.mk>

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib
