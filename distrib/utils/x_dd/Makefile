# $NetBSD: Makefile,v 1.8 1999/06/21 01:19:36 cgd Exp $
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
