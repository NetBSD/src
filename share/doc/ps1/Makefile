#	from: @(#)Makefile	5.1.1.1 (Berkeley) 5/7/91
#	$Id: Makefile,v 1.7 1994/02/10 01:54:16 cgd Exp $

# Missing:  01.Clang 02.f77 03.f77io 05.as 09.lint 10.adb 11.dbx
#	    12.make 15.yacc 16.lex 17.m4
#
# Broken:   04.pascal 18.curses

BINDIR=	/usr/share/doc/ps1
FILES=	00.contents Makefile Title
SUBDIR=	06.sysman 07.ipctut \
	08.ipc 13.rcs 14.sccs

realinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${FILES} \
	    ${DESTDIR}${BINDIR}

.include <bsd.subdir.mk>
