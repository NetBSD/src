# $NetBSD: Makefile,v 1.5.2.1 2010/08/17 06:44:44 uebayasi Exp $

SUBDIR=	bootxx_ffsv1 .WAIT bootxx_ffsv2 bootxx_ustarfs

LIBOBJ= ${.OBJDIR}
.MAKEOVERRIDES+= LIBOBJ

.include <bsd.subdir.mk>
.include <bsd.obj.mk>

cleandir distclean: .WAIT cleanlibdir

cleanlibdir:
	-rm -rf lib
