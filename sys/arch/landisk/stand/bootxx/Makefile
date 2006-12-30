# $NetBSD: Makefile,v 1.2.4.2 2006/12/30 20:46:24 yamt Exp $

SUBDIR=	bootxx_ffsv1

LIBOBJ= ${.OBJDIR}
.MAKEOVERRIDES+= LIBOBJ

.include <bsd.subdir.mk>
.include <bsd.obj.mk>

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib
