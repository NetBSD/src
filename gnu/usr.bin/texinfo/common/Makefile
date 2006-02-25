#	$NetBSD: Makefile,v 1.10 2006/02/25 12:04:23 simonb Exp $

MKPRIVATELIB=	yes

LIB=		common
SRCS+=		substring.c xexit.c xmalloc.c xstrdup.c

CPPFLAGS+=	-DGNULOCALEDIR=\"/usr/share/locale\" \
		-DLOCALE_ALIAS_PATH=\"/usr/share/locale:.\"

.include <bsd.lib.mk>

.PATH: ${IDIST}/lib ${IDIST}/po

all: ${FILES}
.SUFFIXES: .gmo .mo
.gmo.mo:
	@cp $< $@
