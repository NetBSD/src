# $NetBSD: Makefile,v 1.1.1.1 2000/08/09 13:35:23 leo Exp $
# Build an ed without crypt support

PROG=	ed
MKMAN=	no

SRCDIR= ${.CURDIR}/../../../bin/ed

SRCS=	buf.c cbc.c glbl.c io.c main.c re.c sub.c undo.c

CPPFLAGS+= -DBACKWARDS

LDSTATIC?=-static

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
