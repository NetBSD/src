# $NetBSD: Makefile,v 1.2.2.2 2004/08/25 06:57:19 skrll Exp $

SUBDIR=	bootxx_ffsv1 .WAIT bootxx_ffsv2 bootxx_lfsv1 bootxx_lfsv2
SUBDIR+=bootxx_msdos bootxx_ustarfs

LIBOBJ:= ${.OBJDIR}
.MAKEOVERRIDES+= LIBOBJ

.include <bsd.subdir.mk>
.include <bsd.obj.mk>

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib
