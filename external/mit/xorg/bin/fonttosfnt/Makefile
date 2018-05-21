#	$NetBSD: Makefile,v 1.2.40.1 2018/05/21 04:35:53 pgoyette Exp $

.include <bsd.own.mk>

PROG=	fonttosfnt
SRCS=	fonttosfnt.c read.c write.c struct.c util.c

# XXX XVENDORNAME and XVENDORNAMESHORT should live in a common location
CPPFLAGS+=	-DXFREE86_FT2 -I${DESTDIR}${X11INCDIR}/freetype2 \
		-I${DESTDIR}${X11INCDIR}/freetype2/freetype \
		-DXVENDORNAMESHORT=${XVENDORNAMESHORT} \
		-DXVENDORNAME=${XVENDORNAME}
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/fonts


LDADD+=	-lfreetype -lfontenc -lz -lm
DPADD+=	 ${LIBFREETYPE} ${LIBFONTENC} ${LIBZ} ${LIBM}

.PATH:	${X11SRCDIR.${PROG}}

CWARNFLAGS.clang+=	-Wno-format

.include <bsd.x11.mk>
.include <bsd.prog.mk>
