#	from: @(#)sys.mk	5.11 (Berkeley) 3/13/91
#	$Id: sys.mk,v 1.10 1993/12/30 18:53:02 jtc Exp $

unix=		We run UNIX.

.SUFFIXES: .out .a .ln .o .c .cc .C .F .f .e .r .y .l .s .S .cl .p .h .sh
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
YFLAGS=		-d

# single suffix rules
#.c:
#	${CC} ${CFLAGS} ${.IMPSRC} ${LDLIBS} -o ${.TARGET}
#
#.cc .C:
#	${CXX} ${CXXFLAGS} ${.IMPSRC} ${LDLIBS} -o ${.TARGET}
#
#.e .r .F .f:
#	${FC} ${RFLAGS} ${EFLAGS} ${FFLAGS} ${.IMPSRC} ${LDLIBS} -o ${.TARGET}
#
#.p:
#	${PC} ${PFLAGS} ${.IMPSRC} ${LDLIBS} -o ${.TARGET}
#
#.y:
#	${YACC} ${YFLAGS} ${.IMPSRC}
#	${CC} ${CFLAGS} y.tab.c ${LDLIBS} -ly -o ${.TARGET}
#	rm -f y.tab.c
#
#.l:
#	${LEX} ${LFLAGS} ${.IMPSRC}
#	${CC} ${CFLAGS} lex.yy.c ${LDLIBS} -ll -o ${.TARGET}
#	rm -f lex.yy.c
#
#.sh:
#	rm -f ${.TARGET}
#	cp ${.IMPSRC} ${.TARGET}


# double suffix rules
.c.o:
	${CC} ${CFLAGS} -c ${.IMPSRC}

.cc.o .C.o:
	${CXX} ${CXXFLAGS} -c ${.IMPSRC}

.e.o .r.o .F.o .f.o:
	${FC} ${RFLAGS} ${EFLAGS} ${FFLAGS} -c ${.IMPSRC}

.p.o:
	${PC} ${PFLAGS} -c ${.IMPSRC}

.s.o:
	${AS} ${AFLAGS} -o ${.TARGET} ${.IMPSRC}

.S.o:
	${CC} -E ${CFLAGS} ${AFLAGS} ${.IMPSRC} | as -o ${.TARGET}

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

.c.a:
	${CC} -c ${CFLAGS} $<
	${AR} ${ARFLAGS} $@ $*.o
	rm -f $*.o

.f.a:
	${CC} -c ${FFLAGS} $<
	${AR} ${ARFLAGS} $@ $*.o
	rm -f $*.o
	

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
