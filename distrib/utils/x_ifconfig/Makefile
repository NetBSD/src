# $NetBSD: Makefile,v 1.1.1.1 1995/10/08 23:08:47 gwr Exp $
# Build a smaller ifconfig (i.e. for boot media)

PROG=	ifconfig
NOMAN=	

SRCDIR=/usr/src/sbin/ifconfig
CFLAGS+= -DINET_ONLY -I${SRCDIR}

.PATH:  ${SRCDIR}

all: ${PROG}

.include <bsd.prog.mk>
