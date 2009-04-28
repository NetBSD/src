# $NetBSD: Makefile,v 1.2.66.1 2009/04/28 07:34:16 skrll Exp $

SUBDIR=	bootxx_ffsv1 bootxx_ffsv2 bootxx_ustarfs

LIBOBJ= ${.OBJDIR}
.MAKEOVERRIDES+= LIBOBJ

.include <bsd.subdir.mk>
.include <bsd.obj.mk>

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib
