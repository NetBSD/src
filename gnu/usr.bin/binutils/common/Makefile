#	$NetBSD: Makefile,v 1.1.1.1 1997/12/18 07:37:37 thorpej Exp $

LIB=	common

SRCS=	arparse.c arlex.c arsup.c bucomm.c version.c filemode.c rddbg.c \
	debug.c stabs.c ieee.c rdcoff.c wrstabs.c

CPPFLAGS+=-DVERSION='"${VERSION}"' -DTARGET='"${TARGET}"'

NOPROFILE=
NOPIC=

# only needed during build
libinstall::

.include <bsd.lib.mk>
