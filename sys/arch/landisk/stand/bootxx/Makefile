# $NetBSD: Makefile,v 1.1.2.2 2006/09/03 15:23:20 yamt Exp $

SUBDIR=	bootxx_ffsv1

LIBOBJ:= ${.OBJDIR}
.MAKEOVERRIDES+= LIBOBJ

.include <bsd.subdir.mk>
.include <bsd.obj.mk>

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib
