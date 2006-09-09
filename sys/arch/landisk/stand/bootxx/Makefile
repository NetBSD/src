# $NetBSD: Makefile,v 1.1.6.2 2006/09/09 02:40:57 rpaulo Exp $

SUBDIR=	bootxx_ffsv1

LIBOBJ:= ${.OBJDIR}
.MAKEOVERRIDES+= LIBOBJ

.include <bsd.subdir.mk>
.include <bsd.obj.mk>

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib
