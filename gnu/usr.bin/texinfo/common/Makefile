#	$NetBSD: Makefile,v 1.7 2001/12/12 01:49:06 tv Exp $

LIB=		common
SRCS+=		substring.c xexit.c xmalloc.c xstrdup.c

CPPFLAGS+=	-DGNULOCALEDIR=\"/usr/share/locale\" \
		-DLOCALE_ALIAS_PATH=\"/usr/share/locale:.\"

NOLINT=		# defined
NOPROFILE=	# defined
NOPIC=		# defined
NOLINKLIB=	# defined

.include <bsd.lib.mk>

.PATH: ${DIST}/texinfo/lib ${DIST}/texinfo/po

all: ${FILES}
.SUFFIXES: .gmo .mo
.gmo.mo:
	@cp $< $@
