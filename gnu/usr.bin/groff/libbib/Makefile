#	$NetBSD: Makefile,v 1.8 1997/10/18 05:59:06 lukem Exp $

LIB=		bib
SRCS=		common.cc index.cc linear.cc search.cc map.c
CFLAGS+=	-I$(.CURDIR)/../include

NOMAN=		noman
NOPROFILE=	noprofile
NOPIC=		nopic

# only needed during build
libinstall::

.include "../Makefile.cfg"
.include <bsd.lib.mk>
