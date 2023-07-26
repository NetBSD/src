# $NetBSD: Makefile,v 1.3 2023/07/26 02:26:11 rin Exp $
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
COPTS.ffs_appleufs.c+=	-Wno-error=address-of-packed-member
.else
CPPFLAGS+=	-DNO_APPLE_UFS
.endif

CPPFLAGS+=	-DSMALL
CPPFLAGS+=	-DNO_IOBUF_ALIGNED

.PATH:		${SRCDIR}

.include "${SRCDIR}/Makefile.common"

.include <bsd.prog.mk>
