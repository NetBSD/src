PROG=	kbdio
SRCS=	kbdio.y lex.l

#YACC=	bison
#YFLAGS+= -yd	# Bison only

YFLAGS+= -v	# verbose
LFLAGS+= -I

LDADD=	-lm -ly -ll
DPADD=	${LIBM} ${LIBY} ${LIBL}

MKMAN=	no
MKPIC=	no

CFLAGS+= -I. #-g
YFLAGS+= -d

CLEANFILES+= y.tab.h
CLEANFILES+= y.output	# comment file from bison

.include <bsd.prog.mk>

lex.o: y.tab.h lex.l
