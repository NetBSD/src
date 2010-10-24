# $NetBSD: Makefile,v 1.7.70.1 2010/10/24 22:48:04 jym Exp $

SUBDIR= biosboot

LIBOBJ= ${.OBJDIR}
.MAKEOVERRIDES+= LIBOBJ

.include <bsd.subdir.mk>
.include <bsd.obj.mk>

cleandir distclean: .WAIT cleanlibdir

cleanlibdir:
	-rm -rf lib
