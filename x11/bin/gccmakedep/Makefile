#	$NetBSD: Makefile,v 1.1 2003/09/15 08:31:03 lukem Exp $

.include <bsd.own.mk>

CPPSCRIPTS=	gccmakedep
CPPSCRIPTFLAGS=	-DCCCMD='"gcc"' -DRMCMD='"rm -f"' -DLNCMD='"ln -s"' \
		-DMVCMD='"mv -f"'
SCRIPTS=	${CPPSCRIPTS}
MAN=		gccmakedep.1

LNFILES=	gccmdep.cpp gccmakedep.cpp

.PATH:	${X11SRCDIR.xc}/config/util

.include <bsd.x11.mk>
.include <bsd.prog.mk>
