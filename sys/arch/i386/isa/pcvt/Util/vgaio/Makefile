PROG=	vgaio
MAN8=	vgaio.${MAN8EXT}

SRCS=	vgaio.y lex.l
YACC=	yacc

#YFLAGS+= -yd	# Bison
#YFLAGS+= -v	# verbose

LFLAGS+= -I
LDADD=	-lm -ly -ll
DPADD=	${LIBM} ${LIBY} ${LIBL}

CFLAGS+= -I. -g

CLEANFILES+= y.tab.h
CLEANFILES+= y.output	# comment file from bison

NOMAN=	yes

.include <bsd.prog.mk>
