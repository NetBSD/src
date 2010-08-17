# $NetBSD: Makefile,v 1.7.76.1 2010/08/17 06:44:39 uebayasi Exp $

SUBDIR= biosboot

LIBOBJ= ${.OBJDIR}
.MAKEOVERRIDES+= LIBOBJ

.include <bsd.subdir.mk>
.include <bsd.obj.mk>

cleandir distclean: .WAIT cleanlibdir

cleanlibdir:
	-rm -rf lib
