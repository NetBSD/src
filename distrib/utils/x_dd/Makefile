# $NetBSD: Makefile,v 1.10 2000/05/18 01:10:27 matt Exp $
# Build a smaller dd (i.e. for boot media)

PROG=	dd
MKMAN=	no

SRCDIR= ${.CURDIR}/../../../bin/dd

SRCS=	args.c conv.c dd.c misc.c position.c
# SRCS+= conv_tab.c (not used)

CPPFLAGS+= -DNO_CONV -I${SRCDIR}
COPTS+= -Os
LDSTATIC?=-static

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
