#	$NetBSD: Makefile,v 1.5 2001/07/25 16:47:50 assar Exp $

LIB=		common
SRCS+=		getopt.c getopt1.c substring.c xexit.c xmalloc.c xstrdup.c

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
