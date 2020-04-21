# $NetBSD: Makefile,v 1.1.14.1 2020/04/21 18:41:54 martin Exp $
# Build a smaller fsck_ffs (i.e. for boot media).
# Support for Endian-Independent FFS and Apple UFS is dropped unless FFS_EI=1
# and APPLE_UFS=1 are added to CRUNCHENV, respectively.

NOMAN=		# defined

.include <bsd.own.mk>

SRCDIR=		${.CURDIR}/../../../sbin/fsck_ffs

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

CPPFLAGS+=	-DNO_IOBUF_ALIGNED

.PATH:		${SRCDIR}

.include "${SRCDIR}/Makefile.common"

.include <bsd.prog.mk>
