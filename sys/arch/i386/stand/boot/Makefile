# $NetBSD: Makefile,v 1.6.20.1 2006/11/18 21:29:20 ad Exp $

SUBDIR= biosboot

LIBOBJ= ${.OBJDIR}
.MAKEOVERRIDES+= LIBOBJ

.include <bsd.subdir.mk>
.include <bsd.obj.mk>

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib
