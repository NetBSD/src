#	$NetBSD: Makefile,v 1.10.2.1 1997/11/08 06:53:49 lukem Exp $

LIB=		driver
SRCS=		input.cc printer.cc
CPPFLAGS+=	-I$(.CURDIR)/../include

NOMAN=		noman
NOPROFILE=	noprofile
NOPIC=		nopic

# only needed during build
libinstall::

.include "../Makefile.cfg"
.include <bsd.lib.mk>
