# $NetBSD: Makefile,v 1.3 2003/10/08 18:51:50 dsl Exp $

SUBDIR= biosboot

LIBOBJ= ${.OBJDIR}
.MAKEOVERRIDES+= LIBOBJ

.include <bsd.subdir.mk>

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib
