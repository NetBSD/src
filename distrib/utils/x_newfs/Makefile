# $NetBSD: Makefile,v 1.1 2017/02/08 16:11:40 rin Exp $
# Build a smaller newfs (i.e. for boot media).
# Support for Endian-Independent FFS and Apple UFS is dropped unless FFS_EI=1
# and APPLE_UFS=1 are added to CRUNCHENV, respectively.

NOMAN=		# defined

.include <bsd.own.mk>

SRCDIR=		${.CURDIR}/../../../sbin/newfs

.ifdef FFS_EI
SRCS+=		ffs_bswap.c
.else
CPPFLAGS+=	-DNO_FFS_EI
.endif

.ifdef APPLE_UFS
SRCS+=		ffs_appleufs.c
.else
CPPFLAGS+=	-DNO_APPLE_UFS
.endif

.PATH:		${SRCDIR}

.include "${SRCDIR}/Makefile.common"

.include <bsd.prog.mk>
