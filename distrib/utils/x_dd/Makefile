# $NetBSD: Makefile,v 1.1.1.1 1995/10/08 23:08:47 gwr Exp $
# Build a smaller dd (i.e. for boot media)

PROG=	dd
NOMAN=	

SRCDIR=/usr/src/bin/dd

SRCS=	args.c conv.c dd.c misc.c position.c
# SRCS+= conv_tab.c (not used)

CFLAGS+= -DNO_CONV -I${SRCDIR}

.PATH:  ${SRCDIR}

all: ${PROG}

.include <bsd.prog.mk>
