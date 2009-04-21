# $NetBSD: Makefile,v 1.3 2009/04/21 15:17:58 tsutsui Exp $

SUBDIR=	bootxx_ffsv1 bootxx_ffsv2

LIBOBJ= ${.OBJDIR}
.MAKEOVERRIDES+= LIBOBJ

.include <bsd.subdir.mk>
.include <bsd.obj.mk>

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib
