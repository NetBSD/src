#	$NetBSD: Makefile,v 1.3 2001/09/02 22:16:58 wiz Exp $

PROG=		info
SRCS=		dir.c display.c doc.c dribble.c echo-area.c filesys.c \
		footnotes.c gc.c indices.c info-utils.c info.c infodoc.c \
		infomap.c m-x.c man.c nodemenu.c nodes.c search.c session.c \
		signals.c terminal.c tilde.c variables.c window.c
CPPFLAGS+=	-I${DIST}/texinfo/info \
		-DDEFAULT_INFOPATH=\"/usr/pkg/info:/usr/X11R6/info:/usr/X11/info:/usr/local/info:/usr/share/info:/usr/info\"
LDADD+=		-ltermcap
DPADD+=		${LIBTERMCAP}

.include <bsd.prog.mk>

.PATH: ${DIST}/texinfo/info ${DIST}/texinfo/doc
