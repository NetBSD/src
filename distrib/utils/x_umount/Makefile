# $NetBSD: Makefile,v 1.2 2001/08/20 12:19:46 martin Exp $
#
# Build umount(8) without network support
#

SRCDIR= ${.CURDIR}/../../../sbin/umount
PROG=	umount
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
