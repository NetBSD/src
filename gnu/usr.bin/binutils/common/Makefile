#	$NetBSD: Makefile,v 1.2 1998/05/21 20:52:21 tv Exp $

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
