# $NetBSD: Makefile,v 1.6 2010/05/27 06:58:14 dholland Exp $

SUBDIR=	bootxx_ffsv1 .WAIT bootxx_ffsv2 bootxx_ustarfs

LIBOBJ= ${.OBJDIR}
.MAKEOVERRIDES+= LIBOBJ

.include <bsd.subdir.mk>
.include <bsd.obj.mk>

cleandir distclean: .WAIT cleanlibdir

cleanlibdir:
	-rm -rf lib
