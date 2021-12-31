#	$NetBSD: Makefile,v 1.10 2021/12/31 16:40:59 christos Exp $

.include <bsd.own.mk>

INSTIMGBASE=	NetBSD-${DISTRIBVER}-evbarm-${MACHINE_ARCH}-install	# gives ${IMGBASE}.img

INSTIMAGEMB?=	1650			# for all installation binaries

.if !empty(MACHINE_ARCH:Maarch64*)
EFIBOOT=		${WORKDIR}/usr/mdec/bootaa64.efi
KERN_SET=		kern-GENERIC64
.elif !empty(MACHINE_ARCH:Mearmv7hf*)
EFIBOOT+=		${WORKDIR}/usr/mdec/bootarm.efi
KERN_SET=		kern-GENERIC
.endif

USE_GPT=		yes

DISTRIBDIR!= cd ${.CURDIR}/../.. ; pwd
SYSINSTDIR!= cd ${.CURDIR}/../../../usr.sbin/sysinst/arch/${MACHINE} && ${PRINTOBJDIR}

SPEC_EXTRA=		${.CURDIR}/spec.inst
IMGFILE_EXTRA=								\
	${.CURDIR}/etc.ttys		etc/ttys			\
	${.CURDIR}/etc.rc		etc/rc				\
	${.CURDIR}/install.sh		.				\
	${SYSINSTDIR}/sysinstmsgs.de	.				\
	${SYSINSTDIR}/sysinstmsgs.es	.				\
	${SYSINSTDIR}/sysinstmsgs.fr	.				\
	${SYSINSTDIR}/sysinstmsgs.pl	.				\
	${SYSINSTDIR}/sysinst		.

MD_IMGDIR_EXCLUDE=	-s ',./binary/gzimg.*,,gp'
MD_IMGDIR_EXCLUDE+=	-s ',./installation/instkernel.*,,gp'
MD_IMGDIR_EXCLUDE+=	-s ',./installation/ramdisk.*,,gp'

.include "${NETBSDSRCDIR}/distrib/common/bootimage/Makefile.installimage"
