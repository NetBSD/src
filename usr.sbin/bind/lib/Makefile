#	$NetBSD: Makefile,v 1.2 1999/02/13 02:54:58 lukem Exp $

LIB=bind

MKPIC=no
MKPROFILE=no
MKLINT=no

CPPFLAGS+=	${INCLUDE}

.include "${.CURDIR}/inet/Makefile.inc"
.include "${.CURDIR}/isc/Makefile.inc"
.include "${.CURDIR}/nameser/Makefile.inc"
.include "${.CURDIR}/resolv/Makefile.inc"

# only needed during build
libinstall::

.include <bsd.lib.mk>
