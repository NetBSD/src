#	$NetBSD: Makefile,v 1.1 2003/12/23 10:04:14 rtr Exp $

XMODULEDIR=     ${X11USRLIBDIR}/modules/fonts

.include "../../Makefile.serverlib"

.include <bsd.own.mk>

LIB=	type1
.PATH:	${X11SRCDIR.xc}/lib/font/Type1
.PATH:	${X11SRCDIR.xc}/lib/font/Type1/module

SRCS=	arith.c curves.c fontfcn.c hints.c lines.c objects.c paths.c \
	regions.c scanfont.c spaces.c t1funcs.c t1info.c t1io.c t1malloc.c \
	t1snap.c t1stub.c token.c type1.c util.c cidchar.c afm.c \
	t1unicode.c type1mod.c

XSERVERINCDIR!= cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}

CPPFLAGS+=	-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/lib/font/include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${XSERVERINCDIR}

CPPFLAGS+=	-DBUILDCID -DHAVE_CFM -DUSE_MMAP -DFONTMODULE

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
