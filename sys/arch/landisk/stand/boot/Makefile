# $NetBSD: Makefile,v 1.2.78.1 2010/08/17 06:44:44 uebayasi Exp $

SUBDIR= boot

LIBOBJ= ${.OBJDIR}
.MAKEOVERRIDES+= LIBOBJ

.include <bsd.subdir.mk>
.include <bsd.obj.mk>

cleandir distclean: .WAIT cleanlibdir

cleanlibdir:
	-rm -rf lib
