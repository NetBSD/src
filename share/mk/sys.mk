#	from: @(#)sys.mk	5.11 (Berkeley) 3/13/91
#	$Id: sys.mk,v 1.6 1993/08/15 20:42:45 mycroft Exp $

unix=		We run UNIX.

.SUFFIXES: .out .a .ln .o .c .cc .C .F .f .e .r .y .l .s .cl .p .h 
.SUFFIXES: .0 .1 .2 .3 .4 .5 .6 .7 .8

.LIBS:		.a

AR=		ar
ARFLAGS=	rl
RANLIB=		ranlib

AS=		as
AFLAGS=

CC=		cc
CFLAGS=		-O

CXX=		g++
CXXFLAGS=	${CFLAGS}

CPP=		cpp

FC=		f77
FFLAGS=		-O
EFLAGS=

LEX=		lex
LFLAGS=

LD=		ld
LDFLAGS=

LINT=		lint
LINTFLAGS=	-chapbx

MAKE=		make

PC=		pc
PFLAGS=

RC=		f77
RFLAGS=

SHELL=		sh

YACC=		yacc
YFLAGS=-d

.c.o:
	${CC} ${CFLAGS} -c ${.IMPSRC}

.cc.o .C.o:
	${CXX} ${CXXFLAGS} -c ${.IMPSRC}

.p.o:
	${PC} ${PFLAGS} -c ${.IMPSRC}

.e.o .r.o .F.o .f.o:
	${FC} ${RFLAGS} ${EFLAGS} ${FFLAGS} -c ${.IMPSRC}

.s.o:
	${AS} ${AFLAGS} -o ${.TARGET} ${.IMPSRC}

.y.o:
	${YACC} ${YFLAGS} ${.IMPSRC}
	${CC} ${CFLAGS} -c y.tab.c -o ${.TARGET}
	rm -f y.tab.c

.l.o:
	${LEX} ${LFLAGS} ${.IMPSRC}
	${CC} ${CFLAGS} -c lex.yy.c -o ${.TARGET}
	rm -f lex.yy.c

.y.c:
	${YACC} ${YFLAGS} ${.IMPSRC}
	mv y.tab.c ${.TARGET}

.l.c:
	${LEX} ${LFLAGS} ${.IMPSRC}
	mv lex.yy.c ${.TARGET}

.s.out .c.out .o.out:
	${CC} ${CFLAGS} ${.IMPSRC} ${LDLIBS} -o ${.TARGET}

.f.out .F.out .r.out .e.out:
	${FC} ${EFLAGS} ${RFLAGS} ${FFLAGS} ${.IMPSRC} \
	    ${LDLIBS} -o ${.TARGET}
	rm -f ${.PREFIX}.o

.y.out:
	${YACC} ${YFLAGS} ${.IMPSRC}
	${CC} ${CFLAGS} y.tab.c ${LDLIBS} -ly -o ${.TARGET}
	rm -f y.tab.c

.l.out:
	${LEX} ${LFLAGS} ${.IMPSRC}
	${CC} ${CFLAGS} lex.yy.c ${LDLIBS} -ll -o ${.TARGET}
	rm -f lex.yy.c

.8.0 .7.0 .6.0 .5.0 .4.0 .3.0 .2.0 .1.0:
        nroff -mandoc ${.IMPSRC} > ${.TARGET}

.include <bsd.own.mk>
