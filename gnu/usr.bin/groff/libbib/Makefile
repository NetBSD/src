#	$NetBSD: Makefile,v 1.10 1999/02/13 02:54:27 lukem Exp $

LIB=		bib
SRCS=		common.cc index.cc linear.cc search.cc map.c
CPPFLAGS+=	-I$(.CURDIR)/../include

MKMAN=		no
MKPROFILE=	no
MKPIC=		no

# only needed during build
libinstall::

.include "../Makefile.cfg"
.include <bsd.lib.mk>
