#	$NetBSD: Makefile,v 1.5 1999/02/06 05:27:43 tv Exp $

LIB=	common

SRCS=	arparse.y arlex.l arsup.c bucomm.c version.c filemode.c rddbg.c \
	debug.c stabs.c ieee.c rdcoff.c wrstabs.c
YHEADER=1

CPPFLAGS+=-DTARGET='"${TARGET}"'

NOPROFILE=
NOPIC=
NOLINT=

# only needed during build
libinstall::

.include <bsd.lib.mk>
