#	$NetBSD: Makefile,v 1.3 1998/07/29 19:18:26 thorpej Exp $

# XXX until our binutils is upgraded
.if (${MACHINE_ARCH} != "powerpc")
LIB=	common

SRCS=	arparse.y arlex.l arsup.c bucomm.c version.c filemode.c rddbg.c \
	debug.c stabs.c ieee.c rdcoff.c wrstabs.c
.endif

YHEADER=1

CPPFLAGS+=-DVERSION='"${VERSION}"' -DTARGET='"${TARGET}"'

NOPROFILE=
NOPIC=
NOLINT=

# only needed during build
libinstall::

.include <bsd.lib.mk>
