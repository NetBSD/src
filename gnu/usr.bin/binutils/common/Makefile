#	$NetBSD: Makefile,v 1.3.2.2 1998/11/07 00:57:06 cgd Exp $

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
