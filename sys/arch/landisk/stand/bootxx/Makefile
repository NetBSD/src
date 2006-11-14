# $NetBSD: Makefile,v 1.2 2006/11/14 14:09:54 drochner Exp $

SUBDIR=	bootxx_ffsv1

LIBOBJ= ${.OBJDIR}
.MAKEOVERRIDES+= LIBOBJ

.include <bsd.subdir.mk>
.include <bsd.obj.mk>

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib
