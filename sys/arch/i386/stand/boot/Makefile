# $NetBSD: Makefile,v 1.5 2004/08/23 06:54:04 dsl Exp $

SUBDIR= biosboot

LIBOBJ:= ${.OBJDIR}
.MAKEOVERRIDES+= LIBOBJ

.include <bsd.subdir.mk>
.include <bsd.obj.mk>

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib
