#	$NetBSD: Makefile,v 1.4 1999/02/02 22:05:59 tv Exp $

LIB=	common

SRCS=	arparse.y arlex.l arsup.c bucomm.c version.c filemode.c rddbg.c \
	debug.c stabs.c ieee.c rdcoff.c wrstabs.c
YHEADER=1

CPPFLAGS+=-DVERSION='"${VERSION}"' -DTARGET='"${TARGET}"'

NOPROFILE=
NOPIC=
NOLINT=

# only needed during build
libinstall::

.include <bsd.lib.mk>
