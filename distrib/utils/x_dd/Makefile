# $NetBSD: Makefile,v 1.13 2001/11/27 03:33:08 lukem Exp $
# Build a smaller dd (i.e. for boot media)

PROG=	dd
MKMAN=	no

SRCDIR= ${.CURDIR}/../../../bin/dd

SRCS=	args.c conv.c dd.c misc.c position.c strsuftoull.c
# SRCS+= conv_tab.c (not used)

CPPFLAGS+= -DNO_CONV -I${SRCDIR}
.if ${MACHINE_ARCH} != "sparc64"
DBG= -Os
.endif
LDSTATIC?=-static

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
