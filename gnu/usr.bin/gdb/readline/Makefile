# $Id: Makefile,v 1.2 1997/05/07 16:20:34 mycroft Exp $

LIB=		readline
NOPROFILE=
NOPIC=

CFLAGS+=	-DVOID_SIGHANDLER -DVI_MODE
SRCS=		funmap.c history.c keymaps.c readline.c vi_mode.c tilde.c

# only needed during build
libinstall::

.include <bsd.lib.mk>
