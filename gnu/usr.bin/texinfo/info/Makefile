#	$NetBSD: Makefile,v 1.4 2003/01/17 15:32:24 wiz Exp $

PROG=		info
SRCS=		dir.c display.c doc.c dribble.c echo-area.c filesys.c \
		footnotes.c gc.c indices.c info-utils.c info.c infodoc.c \
		infomap.c m-x.c man.c nodemenu.c nodes.c search.c session.c \
		signals.c terminal.c tilde.c variables.c window.c
CPPFLAGS+=	-I${IDIST}/info \
		-DDEFAULT_INFOPATH=\"/usr/pkg/info:/usr/X11R6/info:/usr/X11/info:/usr/local/info:/usr/share/info:/usr/info\"
LDADD+=		-ltermcap
DPADD+=		${LIBTERMCAP}

.include <bsd.prog.mk>

.PATH: ${IDIST}/info ${IDIST}/doc
