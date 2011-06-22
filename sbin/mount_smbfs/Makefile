#	$NetBSD: Makefile,v 1.11 2011/06/22 02:49:44 mrg Exp $

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

# XXX
.if ${HAVE_GCC} == 45
COPTS.rap.c+=	-Wno-error
.endif
