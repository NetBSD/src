#	$NetBSD: Makefile,v 1.2 2023/01/08 09:55:38 mrg Exp $

.include <bsd.own.mk>

PROG=	lndir

CPPFLAGS+=	-DHAVE_STRLCPY -DHAVE_STRLCAT 

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
