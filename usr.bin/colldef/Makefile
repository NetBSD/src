# $Id: Makefile,v 1.1.2.1 2000/05/28 22:41:25 minoura Exp $

PROG = colldef
LFLAGS = -8 -i
CFLAGS += -Wall
CFLAGS += -I${.CURDIR} -I${.CURDIR}/../../lib/libc/locale -DCOLLATE_DEBUG
CFLAGS += -I${.OBJDIR}
CFLAGS += -DYY_NO_UNPUT
SRCS = parse.y scan.l
LDADD = -ll
DPADD = ${LIBL}
YACC= yacc -d

.include <bsd.prog.mk>
