# $NetBSD: Makefile,v 1.12 2000/05/18 01:10:28 matt Exp $
# Build a smaller ifconfig (i.e. for boot media)

PROG=	ifconfig
MKMAN=	no

SRCDIR= ${.CURDIR}/../../../sbin/ifconfig
CPPFLAGS+= -DINET_ONLY -I${SRCDIR}
.if (${MACHINE_ARCH} == "i386") || (${MACHINE_ARCH} == "hpcmips")
CPPFLAGS+= -DINET6
.endif
COPTS+= -Os
LDSTATIC?=-static

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
