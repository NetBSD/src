# $NetBSD: Makefile,v 1.2.2.2 2004/08/25 06:57:19 skrll Exp $

SUBDIR= biosboot

LIBOBJ:= ${.OBJDIR}
.MAKEOVERRIDES+= LIBOBJ

.include <bsd.subdir.mk>
.include <bsd.obj.mk>

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib
