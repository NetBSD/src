#	$NetBSD: Makefile,v 1.10 1997/10/18 05:59:08 lukem Exp $

LIB=		driver
SRCS=		input.cc printer.cc
CFLAGS+=	-I$(.CURDIR)/../include

NOMAN=		noman
NOPROFILE=	noprofile
NOPIC=		nopic

# only needed during build
libinstall::

.include "../Makefile.cfg"
.include <bsd.lib.mk>
