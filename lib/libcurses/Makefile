#	$NetBSD: Makefile,v 1.29 2000/06/03 07:10:31 blymn Exp $
#	@(#)Makefile	8.2 (Berkeley) 1/2/94

CPPFLAGS+=#-DTFILE=\"/dev/ttyp0\"
CPPFLAGS+=-D_CURSES_PRIVATE -I${.CURDIR} -I${.CURDIR}/../libterm
LIB=	curses
SRCS=	addbytes.c addch.c addnstr.c acs.c attributes.c background.c bell.c \
	border.c box.c clear.c clearok.c clrtobot.c clrtoeol.c color.c \
	copywin.c cr_put.c ctrace.c cur_hash.c curses.c curs_set.c delch.c \
        deleteln.c delwin.c erase.c flushok.c fullname.c getch.c getstr.c \
        getyx.c id_subwins.c idlok.c inch.c initscr.c insch.c insdelln.c \
        insertln.c keypad.c leaveok.c line.c longname.c meta.c move.c mvwin.c \
        newwin.c nodelay.c notimeout.c overlay.c overwrite.c pause.c printw.c \
        putchar.c refresh.c scanw.c scroll.c scrollok.c setterm.c standout.c \
        timeout.c toucholap.c touchwin.c tscroll.c tstp.c tty.c unctrl.c \
        underscore.c
MAN=	curses.3
INCS=	curses.h unctrl.h
INCSDIR=/usr/include

CPPFLAGS+=-DCM_N -DCM_GT -DCM_B -DCM_D
SRCS+=  termcap.c tgoto.c tputs.c
.PATH: ${.CURDIR}/../libterm

.if make(install)
SUBDIR+= PSD.doc
.endif

.include <bsd.lib.mk>
.include <bsd.subdir.mk>
