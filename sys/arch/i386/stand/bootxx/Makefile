# $NetBSD: Makefile,v 1.10.20.1 2006/11/18 21:29:20 ad Exp $

SUBDIR=	bootxx_cd9660 bootxx_ffsv1 .WAIT bootxx_ffsv2 bootxx_lfsv1 bootxx_lfsv2
SUBDIR+=bootxx_msdos bootxx_ustarfs

LIBOBJ= ${.OBJDIR}
.MAKEOVERRIDES+= LIBOBJ

.include <bsd.subdir.mk>
.include <bsd.obj.mk>

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib
