# $NetBSD: Makefile,v 1.1 2000/06/20 01:52:11 simonb Exp $
# Build dmesg without kvm support

SRCDIR= ${.CURDIR}/../../../sbin/dmesg
PROG=	dmesg
MKMAN=	no
LDSTATIC?=-static
CPPFLAGS+= -DSMALL
COPTS+= -Os
WARNS?=	1

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
