#	$NetBSD: Makefile,v 1.11 1997/10/25 10:12:06 lukem Exp $

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
