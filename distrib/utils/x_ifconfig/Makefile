# $NetBSD: Makefile,v 1.4.2.1 1997/11/08 05:16:10 lukem Exp $
# Build a smaller ifconfig (i.e. for boot media)

PROG=	ifconfig
NOMAN=	

SRCDIR= ${.CURDIR}/../../../sbin/ifconfig
CPPFLAGS+= -DINET_ONLY -I${SRCDIR}

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
