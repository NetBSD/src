# $NetBSD: Makefile,v 1.6 1999/02/13 02:54:19 lukem Exp $
# Build a smaller dd (i.e. for boot media)

PROG=	dd
MKMAN=	no

SRCDIR= ${.CURDIR}/../../../bin/dd

SRCS=	args.c conv.c dd.c misc.c position.c
# SRCS+= conv_tab.c (not used)

CPPFLAGS+= -DNO_CONV -I${SRCDIR}

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
