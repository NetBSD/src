# $Id: Makefile,v 1.4 1997/05/31 21:21:32 cjs Exp $

LIB=		readline
NOPROFILE=
NOPIC=

CFLAGS+=	-DVOID_SIGHANDLER -DVI_MODE
SRCS=		funmap.c history.c keymaps.c readline.c vi_mode.c tilde.c

# only needed during build
libinstall::

.include <bsd.lib.mk>
