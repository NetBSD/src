#	$NetBSD: Makefile,v 1.9 1997/10/25 10:12:02 lukem Exp $

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
