#	$NetBSD: Makefile,v 1.54 2010/10/26 05:42:51 jruoho Exp $

.include <bsd.own.mk>

# For all platforms

SUBDIR=		accf_dataready
SUBDIR+=	accf_httpready
SUBDIR+=	adosfs
SUBDIR+=	aio
SUBDIR+=	bpf
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
SUBDIR+=	miiverbose
SUBDIR+=	miniroot
SUBDIR+=	mqueue
SUBDIR+=	msdos
SUBDIR+=	nfs
SUBDIR+=	nfsserver
SUBDIR+=	nilfs
SUBDIR+=	ntfs
SUBDIR+=	null
SUBDIR+=	overlay
SUBDIR+=	pciverbose
SUBDIR+=	pf
SUBDIR+=	ppp_bsdcomp
SUBDIR+=	ppp_deflate
SUBDIR+=	procfs
SUBDIR+=	ptyfs
SUBDIR+=	puffs
SUBDIR+=	putter
SUBDIR+=	scsiverbose
SUBDIR+=	secmodel_bsd44
SUBDIR+=	secmodel_overlay
SUBDIR+=	securelevel
SUBDIR+=	smbfs
SUBDIR+=	sysvbfs
SUBDIR+=	suser
SUBDIR+=	swsensor
SUBDIR+=	tmpfs
SUBDIR+=	udf
SUBDIR+=	umap
SUBDIR+=	union
SUBDIR+=	usbverbose
SUBDIR+=	vnd
SUBDIR+=	tprof
.if (defined(NOTYET))
SUBDIR+=	unionfs
.endif
.if ${MKBINUTILS} != "no"
SUBDIR+=	xldscripts
.endif

# Machine dependent section
.if ${MACHINE_ARCH} == "i386" || \
    ${MACHINE_CPU} == "m68k"
SUBDIR+=	exec_aout
.endif

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

.if ${MACHINE_ARCH} == "i386" || \
    ${MACHINE_ARCH} == "ia64" || \
    ${MACHINE_ARCH} == "x86_64"
SUBDIR+=	acpiverbose
.endif

.if ${MACHINE_ARCH} == "i386" || \
    ${MACHINE_ARCH} == "x86_64"
SUBDIR+=	acpiacad
SUBDIR+=	acpibat
SUBDIR+=	acpibut
SUBDIR+=	acpicpu
SUBDIR+=	acpidalb
SUBDIR+=	acpilid
SUBDIR+=	acpitz
SUBDIR+=	acpivga
SUBDIR+=	acpiwmi
SUBDIR+=	aibs
SUBDIR+=	asus
SUBDIR+=	hpqlb
SUBDIR+=	thinkpad
SUBDIR+=	wmidell
SUBDIR+=	wmihp
SUBDIR+=	wmimsi
.endif

.if ${MACHINE_ARCH} == "x86_64"
SUBDIR+=	azalia
SUBDIR+=	compat_linux
SUBDIR+=	compat_linux32
SUBDIR+=	compat_netbsd32
SUBDIR+=	drm
SUBDIR+=	i915drm
SUBDIR+=	pad
.endif

.if ${MACHINE_ARCH} == "i386"
SUBDIR+=	azalia
SUBDIR+=	compat_freebsd
SUBDIR+=	compat_ibcs2
SUBDIR+=	compat_linux
SUBDIR+=	compat_svr4
SUBDIR+=	drm
SUBDIR+=	i915drm
SUBDIR+=	radeondrm
SUBDIR+=	viadrm
SUBDIR+=	pad
.endif

.if ${MACHINE_CPU} == "m68k"
SUBDIR+=	compat_aoutm68k
#SUBDIR+=	compat_linux
#SUBDIR+=	compat_sunos
#SUBDIR+=	compat_svr4
.endif

.include <bsd.own.mk>

# we need our device mapper for LVM
.if (${MKLVM} != "no")
SUBDIR+= 	dm
.endif

.if (${MKDTRACE} != "no")
SUBDIR+=	dtrace
.endif

# we need solaris for the dtrace and zfs modules
.if (${MKDTRACE} != "no" || ${MKZFS} != "no")
SUBDIR+=        solaris
.endif

.if (${MKZFS} != "no")
SUBDIR+=	zfs
.endif

.include <bsd.subdir.mk>
