# $NetBSD: Makefile,v 1.3 2001/10/05 01:17:30 jmc Exp $
# Build dmesg without kvm support

SRCDIR= ${.CURDIR}/../../../sbin/dmesg
PROG=	dmesg
MKMAN=	no
LDSTATIC?=-static
CPPFLAGS+= -DSMALL
.if ${MACHINE_ARCH} != "sparc64"
DBG= -Os
.endif
WARNS?=	1

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
