#	from: @(#)Makefile	5.4 (Berkeley) 6/24/90
#	$NetBSD: Makefile,v 1.1 2009/10/26 00:49:04 christos Exp $
#
# By default, flex will be configured to generate 8-bit scanners only if the
# -8 flag is given.  If you want it to always generate 8-bit scanners, add
# "-DDEFAULT_CSIZE=256" to CPPFLAGS.  Note that doing so will double the size
# of all uncompressed scanners.
# 
# If on your system you have trouble building flex due to 8-bit character
# problems, remove the -8 from FLEX_FLAGS and the "#define FLEX_8_BIT_CHARS"
# from the beginning of flexdef.h.
#
# To bootstrap lex, cp initscan.c to scan.c and run make.

PROG=	lex
CPPFLAGS+=-I. -I${.CURDIR}
SRCS=	 \
buf.c \
ccl.c \
dfa.c \
ecs.c \
filter.c \
gen.c \
main.c \
misc.c \
nfa.c \
options.c \
parse.y \
regex.c \
scan.c \
scanflags.c \
scanopt.c \
skel.c \
sym.c \
tables.c \
tables_shared.c \
tblcmp.c \
yylex.c

YHEADER=1
CLEANFILES+=scan.c skel.c
INCS	=FlexLexer.h
INCSDIR=/usr/include/g++
LDADD+=-lm
DPADD+=${LIBM}

MAN = flex.1

LINKS=	${BINDIR}/lex ${BINDIR}/flex \
	${BINDIR}/lex ${BINDIR}/flex++
MLINKS=	flex.1 lex.1 

skel.c:	mkskel.sh flex.skl
	${_MKTARGET_CREATE}
	${HOST_SH} ${.ALLSRC} >${.TARGET}

.ifndef HOSTPROG
scan.c: scan.l
	${_MKTARGET_LEX}
	${LEX} -t -p ${.ALLSRC} >${.TARGET}
.endif

scan.o yylex.o: parse.h

.include <bsd.prog.mk>
