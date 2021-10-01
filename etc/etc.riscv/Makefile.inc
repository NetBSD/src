#	$NetBSD: Makefile.inc,v 1.2 2021/10/01 10:38:03 jmcneill Exp $
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

.include <bsd.own.mk>
