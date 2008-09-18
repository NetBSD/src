#	$NetBSD: Makefile,v 1.4.32.1 2008/09/18 04:49:46 wrstuden Exp $

.include <bsd.own.mk>

PROG=		info
SRCS=		dir.c display.c dribble.c echo-area.c filesys.c \
		footnotes.c gc.c indices.c info-utils.c info.c infodoc.c \
		infomap.c m-x.c man.c nodemenu.c nodes.c search.c session.c \
		signals.c terminal.c tilde.c variables.c window.c

COMMONDIR!=cd ${.CURDIR}/../common && ${PRINTOBJDIR}

CPPFLAGS+=	-I${IDIST}/info -I${COMMONDIR} \
		-DDEFAULT_INFOPATH=\"/usr/pkg/info:/usr/X11R6/info:/usr/X11/info:/usr/local/info:/usr/share/info:/usr/info\"
LDADD+=		-ltermcap
DPADD+=		${LIBTERMCAP}

.include <bsd.prog.mk>

.PATH: ${IDIST}/info ${IDIST}/doc
