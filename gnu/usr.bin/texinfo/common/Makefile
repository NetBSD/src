#	$NetBSD: Makefile,v 1.10.18.1 2008/05/18 12:30:08 yamt Exp $

LIBISPRIVATE=	yes

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
