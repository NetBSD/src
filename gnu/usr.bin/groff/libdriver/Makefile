#	$NetBSD: Makefile,v 1.12 1999/02/13 02:54:27 lukem Exp $

LIB=		driver
SRCS=		input.cc printer.cc
CPPFLAGS+=	-I$(.CURDIR)/../include

MKMAN=		no
MKPROFILE=	no
MKPIC=		no

# only needed during build
libinstall::

.include "../Makefile.cfg"
.include <bsd.lib.mk>
