#	$NetBSD: Makefile,v 1.1 2003/09/14 07:56:20 lukem Exp $

NOLINT=		1
NOPROFILE=	1
NOSTATICLIB=	1

.include <bsd.own.mk>

.include "../Makefile.libx11"

SRCS+=		XlcDL.c XlcSL.c XDefaultIMIF.c XDefaultOMIF.c

CPPFLAGS.lcInit.c=	$(LC_DYNDEFINES)
CPPFLAGS.lcGeneric.c=	$(LC_DYNDEFINES)
CPPFLAGS.lcPublic.c=	$(LC_DYNDEFINES)
CPPFLAGS.CrCursor.c=	$(XCURSOR_DEFINES)
CPPFLAGS.CrGlCur.c=	$(XCURSOR_DEFINES)
CPPFLAGS.CrPixmap.c=	$(XCURSOR_DEFINES)
CPPFLAGS.PutImage.c=	$(XCURSOR_DEFINES)

.include <bsd.x11.mk>
.include <bsd.lib.mk>
