# $NetBSD: Makefile,v 1.4 2003/10/13 06:56:47 dsl Exp $

SUBDIR= biosboot

LIBOBJ= ${.OBJDIR}
.MAKEOVERRIDES+= LIBOBJ

.include <bsd.subdir.mk>
.include <bsd.obj.mk>

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib
