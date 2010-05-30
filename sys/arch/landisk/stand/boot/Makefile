# $NetBSD: Makefile,v 1.2.80.1 2010/05/30 05:16:56 rmind Exp $

SUBDIR= boot

LIBOBJ= ${.OBJDIR}
.MAKEOVERRIDES+= LIBOBJ

.include <bsd.subdir.mk>
.include <bsd.obj.mk>

cleandir distclean: .WAIT cleanlibdir

cleanlibdir:
	-rm -rf lib
