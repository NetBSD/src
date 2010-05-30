# $NetBSD: Makefile,v 1.7.78.1 2010/05/30 05:16:55 rmind Exp $

SUBDIR= biosboot

LIBOBJ= ${.OBJDIR}
.MAKEOVERRIDES+= LIBOBJ

.include <bsd.subdir.mk>
.include <bsd.obj.mk>

cleandir distclean: .WAIT cleanlibdir

cleanlibdir:
	-rm -rf lib
