#	$NetBSD: Makefile,v 1.4 2000/11/01 02:06:25 itojun Exp $

LIB=		common
SRCS+=		getopt.c getopt1.c xmalloc.c xstrdup.c

CPPFLAGS+=	-DGNULOCALEDIR=\"/usr/share/locale\" \
		-DLOCALE_ALIAS_PATH=\"/usr/share/locale:.\"

MKLINT=		no
MKPROFILE=	no
MKPIC=		no
MKLINKLIB=	no

.include <bsd.lib.mk>

.PATH: ${DIST}/texinfo/lib ${DIST}/texinfo/po

all: ${FILES}
.SUFFIXES: .gmo .mo
.gmo.mo:
	@cp $< $@
