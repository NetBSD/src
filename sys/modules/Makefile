#	$NetBSD: Makefile,v 1.18 2008/11/24 11:26:49 ad Exp $

# For all platforms

SUBDIR=		accf_dataready
SUBDIR+=	accf_httpready
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
SUBDIR+=	exec_script
SUBDIR+=	fdesc
SUBDIR+=	ffs
SUBDIR+=	filecore
SUBDIR+=	fss
SUBDIR+=	hfs
SUBDIR+=	kernfs
SUBDIR+=	ksem
SUBDIR+=	lfs
SUBDIR+=	mfs
SUBDIR+=	msdos
SUBDIR+=	nfs
SUBDIR+=	nfsserver
SUBDIR+=	ntfs
SUBDIR+=	null
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
SUBDIR+=	umap
SUBDIR+=	union
.if (defined(NOTYET))
SUBDIR+=	unionfs
.endif

# Machine dependent section
.if ${MACHINE} != "alpha"
SUBDIR+=	exec_elf32
.endif

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
