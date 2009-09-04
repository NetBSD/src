#	$NetBSD: Makefile,v 1.8 2009/09/04 18:25:56 pooka Exp $

WARNS?=	3	# XXX: sign-compare issues

.include <bsd.own.mk>

PROG=	mount_smbfs
MAN=	mount_smbfs.8
SRCS=	mount_smbfs.c

.if ${MKSHARE} != "no"
.PATH:	${SMBDIST}/examples
FILESDIR=	/usr/share/examples/smbfs
FILES=		dot.nsmbrc
.endif

.include "Makefile.inc"

.include <bsd.prog.mk>
