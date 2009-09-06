#	$NetBSD: Makefile,v 1.10 2009/09/06 18:39:04 pooka Exp $

WARNS?=	3	# XXX: sign-compare issues

.include <bsd.own.mk>

PROG=	mount_smbfs
MAN=	mount_smbfs.8
SRCS=	mount_smbfs.c smb_kernelops.c

.include "Makefile.inc"

.if ${MKSHARE} != "no"
.PATH:	${SMBDIST}/examples
FILESDIR=	/usr/share/examples/smbfs
FILES=		dot.nsmbrc
.endif

.include <bsd.prog.mk>
