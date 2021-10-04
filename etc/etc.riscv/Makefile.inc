#	$NetBSD: Makefile.inc,v 1.3 2021/10/04 06:50:35 skrll Exp $
#
#	etc.riscv/Makefile.inc -- riscv-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC
EXTRA_KERNELS=		INSTALL

.if ${MACHINE_ARCH} == "riscv64" && ${HAVE_UEFI} == "yes"
INSTALLATION_DIRS+=	installation/misc
.endif

snap_md_post:
.if ${MACHINE_ARCH} == "riscv64" && ${HAVE_UEFI} == "yes"
	cd ${KERNSRCDIR}/stand/efiboot/bootriscv64 && ${MAKE} release
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc '*.efi'
.endif
