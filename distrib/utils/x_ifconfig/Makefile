# $NetBSD: Makefile,v 1.2 1995/10/13 18:10:31 gwr Exp $
# Build a smaller ifconfig (i.e. for boot media)

PROG=	ifconfig
NOMAN=	

SRCDIR= ${BSDSRCDIR}/sbin/ifconfig
CFLAGS+= -DINET_ONLY -I${SRCDIR}

.PATH:  ${SRCDIR}

all: ${PROG}

.include <bsd.prog.mk>
