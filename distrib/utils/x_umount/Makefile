# $NetBSD: Makefile,v 1.1 2000/10/30 22:58:02 pk Exp $
#
# Build umount(8) without network support
#

SRCDIR= ${.CURDIR}/../../../sbin/umount
PROG=	umount
MKMAN=	no
LDSTATIC?=-static
CPPFLAGS+= -DSMALL
COPTS+= -Os
WARNS?=	1

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
