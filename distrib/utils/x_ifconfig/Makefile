# $NetBSD: Makefile,v 1.5 1997/10/22 02:16:38 lukem Exp $
# Build a smaller ifconfig (i.e. for boot media)

PROG=	ifconfig
NOMAN=	

SRCDIR= ${.CURDIR}/../../../sbin/ifconfig
CPPFLAGS+= -DINET_ONLY -I${SRCDIR}

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
