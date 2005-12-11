# $NetBSD: Makefile,v 1.6 2005/12/11 12:17:47 christos Exp $

SUBDIR= biosboot

LIBOBJ:= ${.OBJDIR}
.MAKEOVERRIDES+= LIBOBJ

.include <bsd.subdir.mk>
.include <bsd.obj.mk>

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib
