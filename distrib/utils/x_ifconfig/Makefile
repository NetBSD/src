# $NetBSD: Makefile,v 1.3.2.2 1995/10/13 18:35:29 gwr Exp $
# Build a smaller ifconfig (i.e. for boot media)

PROG=	ifconfig
NOMAN=	

SRCDIR= ${BSDSRCDIR}/sbin/ifconfig
CFLAGS+= -DINET_ONLY -I${SRCDIR}

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
