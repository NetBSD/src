# $Id: Makefile,v 1.1.2.2 2000/05/29 14:57:27 minoura Exp $

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
CLEANFILES+=	y.tab.h

.include <bsd.prog.mk>
