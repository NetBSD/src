# $NetBSD: Makefile,v 1.3 2010/05/27 06:58:14 dholland Exp $

SUBDIR= boot

LIBOBJ= ${.OBJDIR}
.MAKEOVERRIDES+= LIBOBJ

.include <bsd.subdir.mk>
.include <bsd.obj.mk>

cleandir distclean: .WAIT cleanlibdir

cleanlibdir:
	-rm -rf lib
