#	$NetBSD: Makefile,v 1.1.1.1 1998/10/05 18:01:57 tron Exp $

LIB=bind

NOPIC=
NOPROFILE=
NOLINT=

CPPFLAGS+=	${INCLUDE}

.include "${.CURDIR}/inet/Makefile.inc"
.include "${.CURDIR}/isc/Makefile.inc"
.include "${.CURDIR}/nameser/Makefile.inc"
.include "${.CURDIR}/resolv/Makefile.inc"

# only needed during build
libinstall::

.include <bsd.lib.mk>
