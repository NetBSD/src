#	$NetBSD: Makefile,v 1.1 1999/02/09 13:19:14 simonb Exp $

LIB=	grep
SRCS=	grep.c dfa.c kwset.c obstack.c savedir.c getopt.c getopt1.c search.c
SRCS+=	stpcpy.c regex.c
CPPFLAGS+=-DHAVE_CONFIG_H

NOLINT=	yes
NOPIC=	yes
NOPROFILE=yes

# only needed during build - prevent installation of library
libinstall::

.include <bsd.lib.mk>
