# $NetBSD: Makefile,v 1.1 2006/09/01 21:26:18 uwe Exp $

SUBDIR= boot

LIBOBJ:= ${.OBJDIR}
.MAKEOVERRIDES+= LIBOBJ

.include <bsd.subdir.mk>
.include <bsd.obj.mk>

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib
