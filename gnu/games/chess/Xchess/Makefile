#	$Id: Makefile,v 1.2 1993/08/02 17:20:42 mycroft Exp $

PROG = xchess

SRCS = board.c button.c clock.c control.c jail.c message.c parse.c popup.c \
       program.c record.c std.c valid.c window.c XCircle.c scrollText.c

CFLAGS += -I. -DDEF_PROGRAM=\"gnuchess\"

LDADD = -L/usr/X386/lib -lX11
DPADD = /usr/X386/lib/libX11.a

.include <bsd.prog.mk>
