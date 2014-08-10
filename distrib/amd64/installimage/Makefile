#	$NetBSD: Makefile,v 1.3.8.1 2014/08/10 06:47:49 tls Exp $

.include <bsd.own.mk>

INSTIMGBASE=	NetBSD-${DISTRIBVER}-amd64-install	# gives ${IMGBASE}.img

BOOTDISK=	sd0			# for USB flash etc.
INSTIMAGEMB?=	550			# for all installation binaries

PRIMARY_BOOT=		bootxx_ffsv1
SECONDARY_BOOT=		boot
SECONDARY_BOOT_ARG=	# unnecessary

USE_MBR=		yes

CLEANFILES+=	boot.cfg

prepare_md_post:
	${TOOL_SED}							\
	    -e "s/@@MACHINE@@/${MACHINE}/"				\
	    -e "s/@@VERSION@@/${DISTRIBVER}/"				\
	    < ${.CURDIR}/boot.cfg.in > boot.cfg

DISTRIBDIR!= cd ${.CURDIR}/../.. ; pwd
SYSINSTDIR!= cd ${.CURDIR}/../../../usr.sbin/sysinst/arch/${MACHINE} && ${PRINTOBJDIR}

SPEC_EXTRA=		${.CURDIR}/spec.inst
IMGFILE_EXTRA=								\
	${DISTRIBDIR}/common/10-resolv.conf	libexec/dhcpcd-hooks	\
	${DISTRIBDIR}/common/99-print-sysinst	libexec/dhcpcd-hooks	\
	${.CURDIR}/etc.ttys		etc/ttys			\
	${.CURDIR}/etc.rc		etc/rc				\
	${.CURDIR}/install.sh		.				\
	${.OBJDIR}/boot.cfg		.				\
	${SYSINSTDIR}/sysinstmsgs.de	.				\
	${SYSINSTDIR}/sysinstmsgs.es	.				\
	${SYSINSTDIR}/sysinstmsgs.fr	.				\
	${SYSINSTDIR}/sysinstmsgs.pl	.				\
	${SYSINSTDIR}/sysinst		.

.include "${DISTRIBDIR}/common/bootimage/Makefile.installimage"
