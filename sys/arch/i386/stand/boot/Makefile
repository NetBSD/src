# $NetBSD: Makefile,v 1.2.2.3 2004/09/18 14:35:50 skrll Exp $

SUBDIR= biosboot

LIBOBJ:= ${.OBJDIR}
.MAKEOVERRIDES+= LIBOBJ

.include <bsd.subdir.mk>
.include <bsd.obj.mk>

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib
