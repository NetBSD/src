# $NetBSD: Makefile,v 1.2 2001/08/20 12:19:44 martin Exp $
# Build dmesg without kvm support

SRCDIR= ${.CURDIR}/../../../sbin/dmesg
PROG=	dmesg
MKMAN=	no
LDSTATIC?=-static
CPPFLAGS+= -DSMALL
.if ${MACHINE_ARCH} != "sparc64"
COPTS+= -Os
.endif
WARNS?=	1

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
