# $NetBSD: Makefile,v 1.7 1999/05/19 04:17:50 gwr Exp $
# Build a smaller ifconfig (i.e. for boot media)

PROG=	ifconfig
MKMAN=	no

SRCDIR= ${BSDSRCDIR}/sbin/ifconfig
CPPFLAGS+= -DINET_ONLY -I${SRCDIR}

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
