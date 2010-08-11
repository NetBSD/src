# $NetBSD: Makefile,v 1.2.58.3 2010/08/11 22:52:17 yamt Exp $

SUBDIR=	bootxx_ffsv1 .WAIT bootxx_ffsv2 bootxx_ustarfs

LIBOBJ= ${.OBJDIR}
.MAKEOVERRIDES+= LIBOBJ

.include <bsd.subdir.mk>
.include <bsd.obj.mk>

cleandir distclean: .WAIT cleanlibdir

cleanlibdir:
	-rm -rf lib
