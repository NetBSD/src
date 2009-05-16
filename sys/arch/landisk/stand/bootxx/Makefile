# $NetBSD: Makefile,v 1.2.58.2 2009/05/16 10:41:15 yamt Exp $

SUBDIR=	bootxx_ffsv1 .WAIT bootxx_ffsv2 bootxx_ustarfs

LIBOBJ= ${.OBJDIR}
.MAKEOVERRIDES+= LIBOBJ

.include <bsd.subdir.mk>
.include <bsd.obj.mk>

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib
