# $NetBSD: Makefile,v 1.10 2000/05/04 03:27:10 itojun Exp $
# Build a smaller ifconfig (i.e. for boot media)

PROG=	ifconfig
MKMAN=	no

SRCDIR= ${.CURDIR}/../../../sbin/ifconfig
CPPFLAGS+= -DINET_ONLY -I${SRCDIR}
.if (${MACHINE_ARCH} == "i386") || (${MACHINE_ARCH} == "hpcmips")
CPPFLAGS+= -DINET6
.endif

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
