#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:16 lukem Exp $

NOMAN=	1

.include <bsd.own.mk>

PROG=	xdmshell

CPPFLAGS.xdmshell.c=	-DHAS_VFORK

.include "../Makefile.xdm"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
