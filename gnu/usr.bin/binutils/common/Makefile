#	$NetBSD: Makefile,v 1.7 1999/02/13 02:54:24 lukem Exp $

LIB=		common

SRCS=		arparse.y arlex.l arsup.c bucomm.c version.c filemode.c \
		rddbg.c debug.c stabs.c ieee.c rdcoff.c wrstabs.c
CPPFLAGS+=	-DTARGET='"${MACHINE_GNU_ARCH}--netbsd"'
YHEADER=1

MKPROFILE=	no
MKPIC=		no
MKLINT=		no

# only needed during build
libinstall::

.include <bsd.lib.mk>
