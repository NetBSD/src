# $NetBSD: Makefile,v 1.5 1997/10/22 02:11:56 lukem Exp $
# Build a smaller dd (i.e. for boot media)

PROG=	dd
NOMAN=	

SRCDIR= ${.CURDIR}/../../../bin/dd

SRCS=	args.c conv.c dd.c misc.c position.c
# SRCS+= conv_tab.c (not used)

CPPFLAGS+= -DNO_CONV -I${SRCDIR}

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
