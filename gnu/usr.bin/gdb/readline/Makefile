# $Id: Makefile,v 1.3 1997/05/30 13:43:17 cjs Exp $

LIB=		readline
NOPROFILE=
NOPIC=

# this library is used only for the build
NOINSTALL=1

CFLAGS+=	-DVOID_SIGHANDLER -DVI_MODE
SRCS=		funmap.c history.c keymaps.c readline.c vi_mode.c tilde.c

.include <bsd.lib.mk>
