#	$NetBSD: Makefile,v 1.264 2022/04/01 21:09:24 pgoyette Exp $

.include <bsd.own.mk>

# For all platforms

# Modules for compatibility with earlier versions of NetBSD

SUBDIR+=	compat_util
SUBDIR+=	compat_43   compat_sysctl_09_43
SUBDIR+=	compat_09   compat_10   compat_12   compat_13   compat_14
SUBDIR+=	compat_16   compat_20   compat_30   compat_40   compat_50
SUBDIR+=	compat_60   compat_70   compat_80   compat_90

SUBDIR+=	compat_sysv_10 compat_sysv_14 compat_sysv_50

SUBDIR+=	compat_50_quota

SUBDIR+=	compat_raid_50
SUBDIR+=	compat_raid_80

SUBDIR+=	compat_crypto_50

SUBDIR+=	accf_dataready
SUBDIR+=	accf_httpready
SUBDIR+=	adosfs
SUBDIR+=	adiantum
SUBDIR+=	aio
SUBDIR+=	audio
SUBDIR+=	autofs
SUBDIR+=	blake2s
SUBDIR+=	blowfish
SUBDIR+=	bpf
SUBDIR+=	bpf_filter
SUBDIR+=	bufq_disksort
SUBDIR+=	bufq_fcfs
SUBDIR+=	bufq_priocscan
SUBDIR+=	bufq_readprio
SUBDIR+=	camellia
SUBDIR+=	cast128
SUBDIR+=	ccd
SUBDIR+=	cd9660
SUBDIR+=	cgd
SUBDIR+=	chfs
SUBDIR+=	clockctl
SUBDIR+=	compat_ossaudio
SUBDIR+=	crypto
SUBDIR+=	coda
SUBDIR+=	coda5
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
SUBDIR+=	flash
SUBDIR+=	fss
SUBDIR+=	gpio
SUBDIR+=	gpioiic
SUBDIR+=	gpioow
SUBDIR+=	gpiosim
SUBDIR+=	gpioirq
SUBDIR+=	gpiopps
SUBDIR+=	hfs
SUBDIR+=	hythygtemp
SUBDIR+=	am2315temp
SUBDIR+=	i2cexec
SUBDIR+=	i2c_bitbang
SUBDIR+=	i2c_subr
SUBDIR+=	if_agr
SUBDIR+=	if_aue
SUBDIR+=	if_axe
SUBDIR+=	if_axen
SUBDIR+=	if_cdce
SUBDIR+=	if_cue
SUBDIR+=	if_faith
SUBDIR+=	if_gif
SUBDIR+=	if_gre
SUBDIR+=	if_kue
SUBDIR+=	if_l2tp
SUBDIR+=	if_lagg
SUBDIR+=	if_loop
SUBDIR+=	if_mpls
SUBDIR+=	if_mue
SUBDIR+=	if_npflog
SUBDIR+=	if_ppp
SUBDIR+=	if_pppoe
SUBDIR+=	if_run
SUBDIR+=	if_sl
SUBDIR+=	if_smsc
SUBDIR+=	if_srt srt
SUBDIR+=	if_stf
SUBDIR+=	if_tap tap
SUBDIR+=	if_tun tun
SUBDIR+=	if_udav
SUBDIR+=	if_upl
SUBDIR+=	if_ure
SUBDIR+=	if_url
SUBDIR+=	if_urndis
SUBDIR+=	if_vlan
SUBDIR+=	if_wg
SUBDIR+=	iic
SUBDIR+=	ip_ecn
SUBDIR+=	ipl
SUBDIR+=	kernfs
SUBDIR+=	layerfs
SUBDIR+=	lfs
SUBDIR+=	lua
SUBDIR+=	luasystm
SUBDIR+=	luapmf
SUBDIR+=	mfs
SUBDIR+=	midi
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
SUBDIR+=	pad
SUBDIR+=	pciverbose
SUBDIR+=	pf
SUBDIR+=	ppp_bsdcomp
SUBDIR+=	ppp_deflate
SUBDIR+=	procfs
SUBDIR+=	ptrace
SUBDIR+=	ptrace_common
SUBDIR+=	ptyfs
SUBDIR+=	puffs
SUBDIR+=	putter
SUBDIR+=	raid
SUBDIR+=	scsiverbose
SUBDIR+=	scmd
SUBDIR+=	scmdi2c
SUBDIR+=	sdtemp
SUBDIR+=	secmodel_bsd44
SUBDIR+=	secmodel_extensions
SUBDIR+=	secmodel_overlay
SUBDIR+=	securelevel
SUBDIR+=	sequencer
SUBDIR+=	sgp40mox
SUBDIR+=	sht3xtemp
SUBDIR+=	sht4xtemp
SUBDIR+=	si70xxtemp
SUBDIR+=	skipjack
SUBDIR+=	slcompress
SUBDIR+=	spdmem
SUBDIR+=	spkr
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
SUBDIR+=	ualea
SUBDIR+=	uatp
SUBDIR+=	udf
SUBDIR+=	ufs
SUBDIR+=	umap
SUBDIR+=	union
SUBDIR+=	usbnet
SUBDIR+=	usbverbose
SUBDIR+=	vcoda
SUBDIR+=	v7fs
SUBDIR+=	vnd
SUBDIR+=	wapbl
SUBDIR+=	wsbell
SUBDIR+=	zlib
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

.if ${MACHINE_ARCH} != "alpha" && \
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
SUBDIR+=	amdsmn
SUBDIR+=	amdtemp
SUBDIR+=	amdzentemp
SUBDIR+=	coretemp
SUBDIR+=	est
SUBDIR+=	hdafg
SUBDIR+=	hdaudio
SUBDIR+=	hdaudio_pci
SUBDIR+=	hdaudioverbose
SUBDIR+=	hpet
SUBDIR+=	ichsmb
SUBDIR+=	odcm
SUBDIR+=	powernow
SUBDIR+=	tco
SUBDIR+=	tprof_x86
SUBDIR+=	vmt
.endif

.if ${HAVE_NVMM:Uno} == "yes"
SUBDIR+=	nvmm
.endif

# Builds on architectures with PCI bus
.if \
     ${MACHINE} == "evbarm" || \
     ${MACHINE_ARCH} == "i386" || \
     ${MACHINE_ARCH} == "x86_64"
#SUBDIR+=	hifn
SUBDIR+=	if_iavf
SUBDIR+=	if_ixl
SUBDIR+=	ubsec
.endif

.if ${MKSLJIT} != "no"
# No modules for 32-bit arm, mips and powerpc yet.
.if ${MACHINE_ARCH} == "aarch64" || \
    ${MACHINE_ARCH} == "i386"    || \
    ${MACHINE_ARCH} == "sparc"   || \
    ${MACHINE_ARCH} == "x86_64"
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

.include "${.CURDIR}/../compat/netbsd32/netbsd32.mk"
.if ${COMPAT_USE_NETBSD32} != "no"

SUBDIR+=	compat_netbsd32
SUBDIR+=	compat_netbsd32_09	compat_netbsd32_12
SUBDIR+=	compat_netbsd32_13	compat_netbsd32_16
SUBDIR+=	compat_netbsd32_20	compat_netbsd32_30
SUBDIR+=	compat_netbsd32_40	compat_netbsd32_50
SUBDIR+=	compat_netbsd32_60	compat_netbsd32_80
SUBDIR+=	compat_netbsd32_90
SUBDIR+=	compat_netbsd32_43
SUBDIR+=	compat_netbsd32_coredump
SUBDIR+=	compat_netbsd32_mqueue
SUBDIR+=	compat_netbsd32_nfssrv
SUBDIR+=	compat_netbsd32_ptrace
SUBDIR+=	compat_netbsd32_quota
SUBDIR+=	compat_netbsd32_quota_50
SUBDIR+=	compat_netbsd32_raid
SUBDIR+=	compat_netbsd32_sysvipc
SUBDIR+=	compat_netbsd32_sysvipc_10
SUBDIR+=	compat_netbsd32_sysvipc_14
SUBDIR+=	compat_netbsd32_sysvipc_50

.if ${MACHINE_ARCH} == "x86_64"
SUBDIR+=	compat_linux32
.endif

.endif

.if ${MACHINE_ARCH} == "i386"
SUBDIR+=	ati_pcigart
SUBDIR+=	compat_freebsd
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
SUBDIR+=	viadrmums
.endif

.if ${MACHINE_ARCH} == "i386" || \
    ${MACHINE_ARCH} == "x86_64"
SUBDIR+=	compat_linux
SUBDIR+=	drm
#SUBDIR+=	drmkms
#SUBDIR+=	drmkms_agp
#SUBDIR+=	drmkms_linux
#SUBDIR+=	drmkms_pci
SUBDIR+=	i915drm
#SUBDIR+=	i915drmkms
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

.if ${MACHINE_CPU} == "aarch64"
SUBDIR+=	compat_linux
SUBDIR+=	compat_linux32
.endif

.if ${MACHINE_CPU} == "m68k"
SUBDIR+=	compat_aoutm68k
#SUBDIR+=	compat_linux
#SUBDIR+=	compat_sunos
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

SUBDIR+=        sodium

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

# Intel Integrated Memory Controller and associated SMBus
# (Experimental)
SUBDIR+=	imc
SUBDIR+=	imcsmb

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
SUBDIR+=	if_aq
SUBDIR+=	if_vioif
SUBDIR+=	if_vmx
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
SUBDIR+=	vio9p
SUBDIR+=	viomb
SUBDIR+=	virtio
SUBDIR+=	virtio_pci
SUBDIR+=	vioscsi
.endif

SUBDIR+=	examples	# build these regularly to avoid bit-rot

.include <bsd.subdir.mk>
