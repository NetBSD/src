# $NetBSD: Makefile,v 1.2.58.1 2009/05/04 08:11:23 yamt Exp $

SUBDIR=	bootxx_ffsv1 bootxx_ffsv2 bootxx_ustarfs

LIBOBJ= ${.OBJDIR}
.MAKEOVERRIDES+= LIBOBJ

.include <bsd.subdir.mk>
.include <bsd.obj.mk>

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib
