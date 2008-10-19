# $NetBSD: Makefile,v 1.7.2.1 2008/10/19 22:17:41 haad Exp $

SUBDIR=		miniroot
SUBDIR+=	fss
SUBDIR+=	putter

.if ${MACHINE} == "i386" || ${MACHINE} == "amd64"
SUBDIR+=	azalia drm i915drm
.endif
.if ${MACHINE} == "i386"
SUBDIR+=	radeondrm
.endif

# file systems
SUBDIR+=	adosfs
SUBDIR+=	cd9660
SUBDIR+=	coda
SUBDIR+=	coda5
SUBDIR+=	efs
SUBDIR+=	ext2fs
SUBDIR+=	fdesc
SUBDIR+=	ffs
SUBDIR+=	filecorefs
SUBDIR+=	hfs
SUBDIR+=	kernfs
SUBDIR+=	lfs
SUBDIR+=	mfs
SUBDIR+=	msdosfs
SUBDIR+=	ntfs
SUBDIR+=	nullfs
SUBDIR+=	overlay
SUBDIR+=	portal
SUBDIR+=	procfs
SUBDIR+=	ptyfs
SUBDIR+=	puffs
SUBDIR+=	smbfs
SUBDIR+=	tmpfs
SUBDIR+=	udf
SUBDIR+=	umapfs
SUBDIR+=	union
.if (defined(NOTYET))
SUBDIR+=	unionfs
.endif

.include <bsd.subdir.mk>
