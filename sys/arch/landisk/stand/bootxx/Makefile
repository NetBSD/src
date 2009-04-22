# $NetBSD: Makefile,v 1.4 2009/04/22 17:20:08 tsutsui Exp $

SUBDIR=	bootxx_ffsv1 bootxx_ffsv2 bootxx_ustarfs

LIBOBJ= ${.OBJDIR}
.MAKEOVERRIDES+= LIBOBJ

.include <bsd.subdir.mk>
.include <bsd.obj.mk>

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib
