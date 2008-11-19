#	$NetBSD: Makefile,v 1.15 2008/11/19 18:36:07 ad Exp $

# For all platforms

SUBDIR=		accf_data
SUBDIR+=	accf_http
SUBDIR+=	adosfs
SUBDIR+=	aio
SUBDIR+=	cd9660
SUBDIR+=	coda
SUBDIR+=	coda5
SUBDIR+=	compat
SUBDIR+=	compat_ossaudio
SUBDIR+=	coredump
SUBDIR+=	efs
SUBDIR+=	ext2fs
SUBDIR+=	exec_elf32
SUBDIR+=	exec_script
SUBDIR+=	fdesc
SUBDIR+=	ffs
SUBDIR+=	filecorefs
SUBDIR+=	fss
SUBDIR+=	hfs
SUBDIR+=	kernfs
SUBDIR+=	ksem
SUBDIR+=	lfs
SUBDIR+=	mfs
SUBDIR+=	msdosfs
SUBDIR+=	nfs
SUBDIR+=	nfsserver
SUBDIR+=	ntfs
SUBDIR+=	nullfs
SUBDIR+=	overlay
SUBDIR+=	portal
SUBDIR+=	procfs
SUBDIR+=	ptyfs
SUBDIR+=	puffs
SUBDIR+=	putter
SUBDIR+=	miniroot
SUBDIR+=	smbfs
SUBDIR+=	tmpfs
SUBDIR+=	udf
SUBDIR+=	umapfs
SUBDIR+=	union
.if (defined(NOTYET))
SUBDIR+=	unionfs
.endif

# Machine dependent section

.if ${MACHINE} == "amd64"
SUBDIR+=	azalia
SUBDIR+=	compat_linux
SUBDIR+=	compat_linux32
SUBDIR+=	compat_netbsd32
SUBDIR+=	drm
SUBDIR+=	exec_elf64
SUBDIR+=	i915drm
.endif

.if ${MACHINE} == "i386"
SUBDIR+=	azalia
SUBDIR+=	compat_freebsd
SUBDIR+=	compat_ibcs2
SUBDIR+=	compat_linux
SUBDIR+=	compat_svr4
SUBDIR+=	drm
SUBDIR+=	exec_aout
SUBDIR+=	i915drm
SUBDIR+=	radeondrm
.endif

.include <bsd.subdir.mk>
