#	$NetBSD: Makefile,v 1.12 2003/07/10 10:34:30 lukem Exp $
#	@(#)Makefile	8.2 (Berkeley) 5/23/94

.include <bsd.own.mk>

# The following modules do not build/install:
# 10.gdb, 13.rcs

# Missing:
# 07.pascal 14.sccs 17.m4

# Missing from 4.4BSD-Lite:
# 01.cacm 02.implement 03.iosys 04.uprog 06.Clang 08.f77 09.f77io
# 11.adb 15.yacc 16.lex

BINDIR=	/usr/share/doc/psd
.if ${MKSHARE} != "no"
FILES=	00.contents Makefile Title
.endif
SUBDIR=	05.sysman 20.ipctut 21.ipc
.if exists(12.make)
SUBDIR+= 12.make
.endif
.if exists(18.gprof)
SUBDIR+= 18.gprof
.endif
.if exists(19.curses)
SUBDIR+= 19.curses
.endif
#SUBDIR=	01.cacm 02.implement 03.iosys 04.uprog 05.sysman 06.Clang \
#	07.pascal 08.f77 09.f77io 11.adb 12.make 14.sccs 15.yacc \
#	16.lex 17.m4 18.gprof 19.curses 20.ipctut 21.ipc

Title.ps: ${FILES}
	${TOOL_ROFF_PS} -ms Title > ${.TARGET}

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
