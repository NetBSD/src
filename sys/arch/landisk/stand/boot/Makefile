# $NetBSD: Makefile,v 1.2.58.1 2010/08/11 22:52:17 yamt Exp $

SUBDIR= boot

LIBOBJ= ${.OBJDIR}
.MAKEOVERRIDES+= LIBOBJ

.include <bsd.subdir.mk>
.include <bsd.obj.mk>

cleandir distclean: .WAIT cleanlibdir

cleanlibdir:
	-rm -rf lib
