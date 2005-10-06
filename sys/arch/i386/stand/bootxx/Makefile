# $NetBSD: Makefile,v 1.8 2005/10/06 08:22:58 dyoung Exp $

SUBDIR=	bootxx_cd9660 .WAIT bootxx_ffsv1 bootxx_ffsv2 bootxx_lfsv1 bootxx_lfsv2
SUBDIR+=bootxx_msdos bootxx_ustarfs

LIBOBJ:= ${.OBJDIR}
.MAKEOVERRIDES+= LIBOBJ

.include <bsd.subdir.mk>
.include <bsd.obj.mk>

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib
