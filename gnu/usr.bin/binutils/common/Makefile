#	$NetBSD: Makefile,v 1.6 1999/02/09 19:21:54 tv Exp $

LIB=		common

SRCS=		arparse.y arlex.l arsup.c bucomm.c version.c filemode.c \
		rddbg.c debug.c stabs.c ieee.c rdcoff.c wrstabs.c
CPPFLAGS+=	-DTARGET='"${MACHINE_GNU_ARCH}--netbsd"'
YHEADER=1

NOPROFILE=
NOPIC=
NOLINT=

# only needed during build
libinstall::

.include <bsd.lib.mk>
