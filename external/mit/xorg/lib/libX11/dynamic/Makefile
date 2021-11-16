#	$NetBSD: Makefile,v 1.5 2021/11/16 09:25:51 nia Exp $

NOLINT=		1
NOPROFILE=	1
NOSTATICLIB=	1

USE_FORT?=	yes # data-driven bugs?

.include <bsd.own.mk>

.include "../Makefile.libx11"
.include "../Makefile.ximcp"
.include "../Makefile.xlcDef"
.include "../Makefile.xlcUTF8Load"
.include "../Makefile.xlibi18n"
.include "../Makefile.xomGeneric"

# Set this to use dynamic i18n support.
#SRCS+=	XlcDL.c XlcSL.c

SRCS+=	${SRCS.ximcp} \
	${SRCS.xlcDef} \
	${SRCS.xlcUTF8Load} \
	${SRCS.xlibi18n} \
	${SRCS.xomGeneric}

CPPFLAGS.lcInit.c=	$(LC_DYNDEFINES)
CPPFLAGS.lcGeneric.c=	$(LC_DYNDEFINES)
CPPFLAGS.lcPublic.c=	$(LC_DYNDEFINES)
CPPFLAGS.CrCursor.c=	$(XCURSOR_DEFINES)
CPPFLAGS.CrGlCur.c=	$(XCURSOR_DEFINES)
CPPFLAGS.CrPixmap.c=	$(XCURSOR_DEFINES)
CPPFLAGS.PutImage.c=	$(XCURSOR_DEFINES)

LIBDPLIBS+= \
	xcb ${.CURDIR}/../../libxcb/libxcb \
	Xdmcp ${.CURDIR}/../../libXdmcp \
	Xau ${.CURDIR}/../../libXau

CWARNFLAGS.clang+=	-Wno-parentheses

.include <bsd.x11.mk>
.include <bsd.lib.mk>
