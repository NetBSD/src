# $NetBSD: Makefile,v 1.1.4.1 2006/11/18 21:29:23 ad Exp $

SUBDIR= boot

LIBOBJ= ${.OBJDIR}
.MAKEOVERRIDES+= LIBOBJ

.include <bsd.subdir.mk>
.include <bsd.obj.mk>

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib
