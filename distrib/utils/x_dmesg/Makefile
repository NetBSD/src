# $NetBSD: Makefile,v 1.1.2.2 2001/07/29 20:32:31 he Exp $
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
