# $NetBSD: Makefile,v 1.2.2.4 2004/09/21 13:17:10 skrll Exp $

SUBDIR= biosboot

LIBOBJ:= ${.OBJDIR}
.MAKEOVERRIDES+= LIBOBJ

.include <bsd.subdir.mk>
.include <bsd.obj.mk>

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib
