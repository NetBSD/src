#	$Id: Makefile,v 1.7 1997/05/07 16:20:36 mycroft Exp $

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
