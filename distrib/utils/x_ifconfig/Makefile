# $NetBSD: Makefile,v 1.15 2001/10/05 01:17:31 jmc Exp $
# Build a smaller ifconfig (i.e. for boot media)

PROG=	ifconfig
MKMAN=	no

SRCDIR= ${.CURDIR}/../../../sbin/ifconfig
CPPFLAGS+= -DINET_ONLY -I${SRCDIR}
.if (${MACHINE} == "i386") || (${MACHINE} == "hpcmips") || (${MACHINE} == "x68k")
CPPFLAGS+= -DINET6
.endif
.if ${MACHINE_ARCH} != "sparc64"
DBG= -Os
.endif
LDSTATIC?=-static

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
