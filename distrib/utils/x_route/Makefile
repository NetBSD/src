#	$NetBSD: Makefile,v 1.9 2000/05/18 01:10:30 matt Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	route
MKMAN=	no
SRCS=	route.c show.c keywords.c

SRCDIR= ${.CURDIR}/../../../sbin/route
CPPFLAGS+= -DSMALL -I${SRCDIR}
.if (${MACHINE_ARCH} == "i386") || (${MACHINE_ARCH} == "hpcmips")
CPPFLAGS+= -DINET6
.endif
COPTS+= -Os
LDSTATIC?=-static

BINOWN=	root
BINMODE=4555

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
