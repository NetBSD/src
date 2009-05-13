#	$NetBSD: Makefile,v 1.26.2.1 2009/05/13 17:22:17 jym Exp $

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
SUBDIR+=	layerfs
SUBDIR+=	lfs
SUBDIR+=	mfs
SUBDIR+=	msdos
SUBDIR+=	nfs
SUBDIR+=	nfsserver
SUBDIR+=	ntfs
SUBDIR+=	null
SUBDIR+=	overlay
SUBDIR+=	portal
SUBDIR+=	ppp_bsdcomp
SUBDIR+=	ppp_deflate
SUBDIR+=	procfs
SUBDIR+=	ptyfs
SUBDIR+=	puffs
SUBDIR+=	putter
SUBDIR+=	miniroot
SUBDIR+=	smbfs
SUBDIR+=	sysvbfs
SUBDIR+=	tmpfs
SUBDIR+=	udf
SUBDIR+=	umap
SUBDIR+=	union
SUBDIR+=	vnd
SUBDIR+=	tprof
.if (defined(NOTYET))
SUBDIR+=	unionfs
.endif

# Machine dependent section
.if ${MACHINE_ARCH} != "alpha"
SUBDIR+=	exec_elf32
.endif

.if ${MACHINE_ARCH} == "alpha" || \
    ${MACHINE_ARCH} == "sparc64" || \
    ${MACHINE_ARCH} == "x86_64"
SUBDIR+=	exec_elf64
.endif

.if ${MACHINE_ARCH} == "i386" || \
    ${MACHINE_ARCH} == "x86_64"
SUBDIR+=	tprof_pmi
.endif

.if ${MACHINE_ARCH} == "x86_64"
SUBDIR+=	azalia
SUBDIR+=	compat_linux
SUBDIR+=	compat_linux32
SUBDIR+=	compat_netbsd32
SUBDIR+=	drm
SUBDIR+=	i915drm
.endif

.if ${MACHINE_ARCH} == "i386"
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

.include <bsd.own.mk>

# we need our device mapper for LVM
.if (${MKLVM} != "no")
SUBDIR+= 	dm
.endif

.include <bsd.subdir.mk>
