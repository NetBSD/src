#	$NetBSD: Makefile,v 1.8.2.1 1997/11/08 06:53:47 lukem Exp $

LIB=		bib
SRCS=		common.cc index.cc linear.cc search.cc map.c
CPPFLAGS+=	-I$(.CURDIR)/../include

NOMAN=		noman
NOPROFILE=	noprofile
NOPIC=		nopic

# only needed during build
libinstall::

.include "../Makefile.cfg"
.include <bsd.lib.mk>
