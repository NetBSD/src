#	from: @(#)Makefile	5.4 (Berkeley) 6/24/90
#	$Id: Makefile,v 1.8 1994/06/02 23:40:06 jtc Exp $
#
# By default, flex will be configured to generate 8-bit scanners only if the
# -8 flag is given.  If you want it to always generate 8-bit scanners, add
# "-DDEFAULT_CSIZE=256" to CFLAGS.  Note that doing so will double the size
# of all uncompressed scanners.
# 
# If on your system you have trouble building flex due to 8-bit character
# problems, remove the -8 from FLEX_FLAGS and the "#define FLEX_8_BIT_CHARS"
# from the beginning of flexdef.h.
#
# To bootstrap lex, cp initscan.c to scan.c and run make.

PROG=	lex
CFLAGS+=-I. -I${.CURDIR}
LFLAGS=	-is8 -p
SRCS=	ccl.c dfa.c ecs.c gen.c main.c misc.c nfa.c parse.c sym.c tblcmp.c \
	yylex.c
OBJS+=	scan.o skel.o
CLEANFILES+=parse.c parse.h scan.c skel.c y.tab.c y.tab.h
LDADD=	-ll
DPADD=	${LIBL}

MAN1 = flex.0 flexdoc.0

LINKS=	${BINDIR}/lex ${BINDIR}/flex \
	${BINDIR}/lex ${BINDIR}/flex++
MLINKS=	flex.1 lex.1 flexdoc.1 lexdoc.1

parse.h parse.c: parse.y
	${YACC} -d ${.IMPSRC}
	mv y.tab.c parse.c
	mv y.tab.h parse.h

skel.c:	flex.skl mkskel.sh
	${.CURDIR}/mkskel.sh ${.CURDIR}/flex.skl > skel.c

scan.o: parse.c

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/FlexLexer.h \
		${DESTDIR}/usr/include/g++

.include <bsd.prog.mk>
