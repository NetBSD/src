# $NetBSD: Makefile,v 1.4.2.1 1997/11/08 05:15:17 lukem Exp $
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
