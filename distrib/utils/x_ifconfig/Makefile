# $NetBSD: Makefile,v 1.13 2000/09/05 16:06:49 minoura Exp $
# Build a smaller ifconfig (i.e. for boot media)

PROG=	ifconfig
MKMAN=	no

SRCDIR= ${.CURDIR}/../../../sbin/ifconfig
CPPFLAGS+= -DINET_ONLY -I${SRCDIR}
.if (${MACHINE} == "i386") || (${MACHINE} == "hpcmips") || (${MACHINE} == "x68k")
CPPFLAGS+= -DINET6
.endif
COPTS+= -Os
LDSTATIC?=-static

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
