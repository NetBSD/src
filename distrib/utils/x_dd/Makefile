# $NetBSD: Makefile,v 1.11 2001/08/20 12:19:44 martin Exp $
# Build a smaller dd (i.e. for boot media)

PROG=	dd
MKMAN=	no

SRCDIR= ${.CURDIR}/../../../bin/dd

SRCS=	args.c conv.c dd.c misc.c position.c
# SRCS+= conv_tab.c (not used)

CPPFLAGS+= -DNO_CONV -I${SRCDIR}
.if ${MACHINE_ARCH} != "sparc64"
COPTS+= -Os
.endif
LDSTATIC?=-static

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
