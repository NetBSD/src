#	$NetBSD: Makefile,v 1.21 2022/01/03 12:10:17 martin Exp $

.include <bsd.own.mk>

INSTIMGBASE=	NetBSD-${DISTRIBVER}-amd64-install	# gives ${IMGBASE}.img

INSTIMAGEMB?=	2250			# for all installation binaries

PRIMARY_BOOT=		bootxx_ffsv1
SECONDARY_BOOT=		boot
SECONDARY_BOOT_ARG=	# unnecessary
EFIBOOT=		${WORKDIR}/usr/mdec/bootx64.efi
EFIBOOT+=		${WORKDIR}/usr/mdec/bootia32.efi
#EFIBOOT=		${.OBJDIR}/../../../sys/arch/i386/stand/efiboot/bootx64/bootx64.efi
#EFIBOOT+=		${.OBJDIR}/../../../sys/arch/i386/stand/efiboot/bootia32/bootia32.efi

USE_MBR=		yes
USE_GPT=		yes
USE_GPTMBR=		yes

CLEANFILES+=	boot.cfg

prepare_md_post:
	${TOOL_SED}							\
	    -e "s/@@MACHINE@@/${MACHINE}/"				\
	    -e "s/@@VERSION@@/${DISTRIBVER}/"				\
	    < ${.CURDIR}/boot.cfg.in > boot.cfg

DISTRIBDIR!= cd ${.CURDIR}/../.. ; pwd

SPEC_EXTRA=		${.CURDIR}/spec.inst
IMGFILE_EXTRA=								\
	${.CURDIR}/etc.ttys		etc/ttys			\
	${.CURDIR}/etc.rc		etc/rc				\
	${.CURDIR}/install.sh		.				\
	${.OBJDIR}/boot.cfg		.

.include "${NETBSDSRCDIR}/distrib/common/bootimage//Makefile.installimage"
