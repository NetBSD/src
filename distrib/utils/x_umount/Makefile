# $NetBSD: Makefile,v 1.3 2001/10/05 01:17:32 jmc Exp $
#
# Build umount(8) without network support
#

SRCDIR= ${.CURDIR}/../../../sbin/umount
PROG=	umount
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
