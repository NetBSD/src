# $NetBSD: Makefile,v 1.1.2.2 2006/09/03 15:23:19 yamt Exp $

SUBDIR= boot

LIBOBJ:= ${.OBJDIR}
.MAKEOVERRIDES+= LIBOBJ

.include <bsd.subdir.mk>
.include <bsd.obj.mk>

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib
