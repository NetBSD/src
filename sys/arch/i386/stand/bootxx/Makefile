# $NetBSD: Makefile,v 1.11.76.1 2010/08/17 06:44:39 uebayasi Exp $

SUBDIR=	bootxx_cd9660 bootxx_ffsv1 .WAIT bootxx_ffsv2 bootxx_lfsv1 bootxx_lfsv2
SUBDIR+=bootxx_msdos bootxx_ustarfs

LIBOBJ= ${.OBJDIR}
.MAKEOVERRIDES+= LIBOBJ

.include <bsd.subdir.mk>
.include <bsd.obj.mk>

cleandir distclean: .WAIT cleanlibdir

cleanlibdir:
	-rm -rf lib
