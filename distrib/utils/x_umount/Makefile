# $NetBSD: Makefile,v 1.1.2.2 2000/11/01 02:56:30 tv Exp $
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
