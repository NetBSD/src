#	$NetBSD: Makefile,v 1.8 1999/10/10 13:02:32 pk Exp $

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

# mention explicit dependency, so arparse.h will appear timely
arlex.c: arparse.h

.include <bsd.lib.mk>
