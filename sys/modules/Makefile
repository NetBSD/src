#	$NetBSD: Makefile,v 1.142.2.6 2016/10/05 20:56:04 skrll Exp $

.include <bsd.own.mk>

# For all platforms

SUBDIR=		accf_dataready
SUBDIR+=	accf_httpready
SUBDIR+=	adosfs
SUBDIR+=	aio
SUBDIR+=	blowfish
SUBDIR+=	bpf
SUBDIR+=	bpf_filter
SUBDIR+=	camellia
SUBDIR+=	cast128
SUBDIR+=	ccd
SUBDIR+=	cd9660
SUBDIR+=	cgd
SUBDIR+=	chfs
SUBDIR+=	clockctl
SUBDIR+=	crypto
SUBDIR+=	coda
SUBDIR+=	coda5
SUBDIR+=	compat
SUBDIR+=	compat_ossaudio
SUBDIR+=	coredump
SUBDIR+=	dbcool
SUBDIR+=	des
SUBDIR+=	dk_subr
SUBDIR+=	drvctl
SUBDIR+=	efs
SUBDIR+=	ext2fs
SUBDIR+=	exec_script
SUBDIR+=	fdesc
SUBDIR+=	ffs
SUBDIR+=	filecore
SUBDIR+=	filemon
SUBDIR+=	flash
SUBDIR+=	fss
SUBDIR+=	gpio
SUBDIR+=	gpioiic
SUBDIR+=	gpioow
SUBDIR+=	gpiosim
SUBDIR+=	hfs
SUBDIR+=	hythygtemp
SUBDIR+=	i2cexec
SUBDIR+=	i2c_bitbang
SUBDIR+=	if_agr
SUBDIR+=	if_axe
SUBDIR+=	if_axen
SUBDIR+=	if_faith
SUBDIR+=	if_gif
SUBDIR+=	if_gre
SUBDIR+=	if_loop
SUBDIR+=	if_mpls
SUBDIR+=	if_npflog
SUBDIR+=	if_ppp
SUBDIR+=	if_pppoe
SUBDIR+=	if_sl
SUBDIR+=	if_smsc
SUBDIR+=	if_srt
SUBDIR+=	if_stf
SUBDIR+=	if_strip
SUBDIR+=	if_tun tun
SUBDIR+=	if_vlan
SUBDIR+=	iic
SUBDIR+=	ipl
SUBDIR+=	kernfs
SUBDIR+=	layerfs
SUBDIR+=	lfs
SUBDIR+=	lua
SUBDIR+=	luasystm
SUBDIR+=	luapmf
SUBDIR+=	mfs
SUBDIR+=	miiverbose
SUBDIR+=	miniroot
SUBDIR+=	mqueue
SUBDIR+=	msdos
SUBDIR+=	nand
SUBDIR+=	nandemulator
SUBDIR+=	nfs
SUBDIR+=	nfsserver
SUBDIR+=	nilfs
SUBDIR+=	npf
SUBDIR+=	npf_alg_icmp
SUBDIR+=	npf_ext_log
SUBDIR+=	npf_ext_normalize
SUBDIR+=	npf_ext_rndblock
SUBDIR+=	ntfs
SUBDIR+=	null
SUBDIR+=	onewire
SUBDIR+=	opencrypto
SUBDIR+=	overlay
SUBDIR+=	pciverbose
SUBDIR+=	pf
SUBDIR+=	ppp_bsdcomp
SUBDIR+=	ppp_deflate
SUBDIR+=	procfs
SUBDIR+=	ptyfs
SUBDIR+=	puffs
SUBDIR+=	putter
SUBDIR+=	raid
SUBDIR+=	scsiverbose
SUBDIR+=	sdtemp
SUBDIR+=	secmodel_bsd44
SUBDIR+=	secmodel_extensions
SUBDIR+=	secmodel_overlay
SUBDIR+=	securelevel
SUBDIR+=	skipjack
SUBDIR+=	slcompress
SUBDIR+=	smbfs
SUBDIR+=	spdmem
SUBDIR+=	sppp_subr
SUBDIR+=	sysmon
SUBDIR+=	sysmon_envsys
SUBDIR+=	sysmon_power
SUBDIR+=	sysmon_taskq
SUBDIR+=	sysmon_wdog
SUBDIR+=	sysvbfs
SUBDIR+=	sysv_ipc
SUBDIR+=	suser
SUBDIR+=	swcrypto
SUBDIR+=	swsensor
SUBDIR+=	swwdog
SUBDIR+=	tmpfs
SUBDIR+=	uatp
SUBDIR+=	udf
SUBDIR+=	umap
SUBDIR+=	union
SUBDIR+=	usbverbose
SUBDIR+=	vcoda
SUBDIR+=	v7fs
SUBDIR+=	vnd
SUBDIR+=	wapbl
SUBDIR+=	zlib
SUBDIR+=	tprof
.if (defined(NOTYET))
SUBDIR+=	hifn		# Builds on architectures with PCI bus
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

.if ${MACHINE_ARCH} != "alpha" && \
    ${MACHINE_CPU} != "aarch64" && \
    ${MACHINE_ARCH} != "ia64"
SUBDIR+=	exec_elf32
.endif

.if ${MACHINE_CPU} == "aarch64" || \
    ${MACHINE_ARCH} == "alpha" || \
    ${MACHINE_ARCH} == "ia64" || \
    ${MACHINE_ARCH} == "sparc64" || \
    ${MACHINE_ARCH} == "x86_64"
SUBDIR+=	exec_elf64
.endif

.if ${MACHINE_ARCH} == "i386" || \
    ${MACHINE_ARCH} == "x86_64"
SUBDIR+=	amdtemp
SUBDIR+=	coretemp
SUBDIR+=	est
SUBDIR+=	hdafg
SUBDIR+=	hdaudio
SUBDIR+=	hdaudio_pci
SUBDIR+=	hdaudioverbose
SUBDIR+=	hpet
SUBDIR+=	odcm
SUBDIR+=	powernow
SUBDIR+=	tco
SUBDIR+=	tprof_pmi
SUBDIR+=	tprof_amdpmi
SUBDIR+=	vmt
.endif

.if ${MACHINE_ARCH} == "i386" || \
    ${MACHINE_ARCH} == "x86_64"
SUBDIR+=	ubsec		# Builds on architectures with PCI bus
.endif

.if ${MKSLJIT} != "no"
# No modules for arm, mips and powerpc yet.
.if ${MACHINE_ARCH} == "i386" || \
    ${MACHINE_ARCH} == "x86_64" || \
    ${MACHINE_ARCH} == "sparc"
SUBDIR+=	bpfjit
SUBDIR+=	sljit
.endif
.endif

#
# ACPI modules
#
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
SUBDIR+=	acpifan
SUBDIR+=	acpilid
SUBDIR+=	acpipmtr
SUBDIR+=	acpitz
SUBDIR+=	acpivga
SUBDIR+=	acpiwdrt
SUBDIR+=	acpiwmi
SUBDIR+=	aibs
SUBDIR+=	asus
SUBDIR+=	fujbp
SUBDIR+=	fujhk
SUBDIR+=	hpacel
SUBDIR+=	hpqlb
SUBDIR+=	thinkpad
SUBDIR+=	wmidell
SUBDIR+=	wmieeepc
SUBDIR+=	wmihp
SUBDIR+=	wmimsi
.endif

.if ${MACHINE_CPU} == "arm" \
    || (!empty(MACHINE_ARCH:Mmips64*) && !defined(BSD_MK_COMPAT_FILE))
SUBDIR+=	compat_netbsd32
SUBDIR+=	compat_netbsd32_nfssrv
SUBDIR+=	compat_netbsd32_mqueue
SUBDIR+=	compat_netbsd32_sysvipc
.endif

.if ${MACHINE_ARCH} == "x86_64"
SUBDIR+=	compat_linux32
SUBDIR+=	compat_netbsd32
SUBDIR+=	compat_netbsd32_nfssrv
SUBDIR+=	compat_netbsd32_mqueue
SUBDIR+=	compat_netbsd32_sysvipc
.endif

.if ${MACHINE_ARCH} == "i386"
SUBDIR+=	ati_pcigart
SUBDIR+=	compat_freebsd
SUBDIR+=	compat_ibcs2
SUBDIR+=	compat_svr4
SUBDIR+=	mach64drm
SUBDIR+=	mgadrm
SUBDIR+=	nsclpcsio
SUBDIR+=	padlock
SUBDIR+=	r128drm
SUBDIR+=	radeondrm
SUBDIR+=	savagedrm
SUBDIR+=	sisdrm
SUBDIR+=	tdfxdrm
SUBDIR+=	viac7temp
SUBDIR+=	viadrm
SUBDIR+=	viadrmums
.endif

.if ${MACHINE_ARCH} == "i386" || \
    ${MACHINE_ARCH} == "x86_64"
SUBDIR+=	azalia
SUBDIR+=	compat_linux
SUBDIR+=	drm
SUBDIR+=	drmkms
SUBDIR+=	drmkms_linux
SUBDIR+=	drmkms_pci
SUBDIR+=	i915drm
SUBDIR+=	i915drmkms
SUBDIR+=	pad
#
# ISA modules
#
SUBDIR+=	aps
SUBDIR+=	finsio
SUBDIR+=	itesio
SUBDIR+=	lm
SUBDIR+=	lm_isa
SUBDIR+=	lm_isa_common
SUBDIR+=	lm_wbsio
SUBDIR+=	wbsio
.endif

.if ${MACHINE_CPU} == "m68k"
SUBDIR+=	compat_aoutm68k
#SUBDIR+=	compat_linux
#SUBDIR+=	compat_sunos
#SUBDIR+=	compat_svr4
.endif

.if ${MACHINE_ARCH} == "i386" || \
    ${MACHINE_ARCH} == "x86_64" || \
    ${MACHINE_ARCH} == "sparc64"
SUBDIR+=	pwdog
.endif

.include <bsd.own.mk>

#
# NetBSD/usermode support
#
.if ${MACHINE_ARCH} == "i386" || \
    ${MACHINE_ARCH} == "x86_64"
SUBDIR+=	../arch/usermode/modules/syscallemu
.endif

# we need our device mapper for LVM
.if (${MKLVM} != "no")
SUBDIR+= 	dm
.endif

.if (${MKDTRACE} != "no")
SUBDIR+=	cyclic
SUBDIR+=	dtrace
.endif

# we need solaris for the dtrace and zfs modules
.if (${MKDTRACE} != "no" || ${MKZFS} != "no")
SUBDIR+=        solaris
.endif

.if (${MKZFS} != "no")
SUBDIR+=	zfs
.endif

.if (${MKCOMPATMODULES} != "no")
SUBDIR+=	arch
.endif

#
# DTV modules
#
SUBDIR+=	dtv
SUBDIR+=	dtv_math
.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
# tuners/demods
SUBDIR+=	au8522
SUBDIR+=	cx24227
SUBDIR+=	lg3303
SUBDIR+=	mt2131
SUBDIR+=	nxt2k
SUBDIR+=	tvpll
SUBDIR+=	xc3028
SUBDIR+=	xc5k
SUBDIR+=	zl10353
# hardware drivers
SUBDIR+=	auvitek
SUBDIR+=	coram
SUBDIR+=	cxdtv
SUBDIR+=	emdtv
SUBDIR+=	spkr
.endif

.if (${MKISCSI} != "no")
SUBDIR+=	iscsi
.endif

# For now, all of these are disabled, pending resolution of how to deal
# with attachment-specific modules and bus-specific modules (as opposed
# to machine- or architecture-specific modules)

.if 0
SUBDIR+=	aac
SUBDIR+=	amr
SUBDIR+=	ataraid
SUBDIR+=	cac
SUBDIR+=	cac_eisa
SUBDIR+=	cac_pci
SUBDIR+=	if_vioif
SUBDIR+=	ld
SUBDIR+=	ld_aac
SUBDIR+=	ld_amr
SUBDIR+=	ld_ataraid
SUBDIR+=	ld_cac
SUBDIR+=	ld_icp
SUBDIR+=	ld_mlx
SUBDIR+=	ld_sdmmc
SUBDIR+=	ld_twa
SUBDIR+=	ld_twe
SUBDIR+=	ld_virtio
SUBDIR+=	mlx
SUBDIR+=	mlx_eisa
SUBDIR+=	mlx_pci
SUBDIR+=	ld_nvme
SUBDIR+=	nvme
SUBDIR+=	twa
SUBDIR+=	twe
SUBDIR+=	viomb
SUBDIR+=	virtio
.endif

SUBDIR+=	examples	# build these regularly to avoid bit-rot

.include <bsd.subdir.mk>
