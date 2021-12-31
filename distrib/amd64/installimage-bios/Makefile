#	$NetBSD: Makefile,v 1.4 2021/12/31 04:59:15 pgoyette Exp $

.include <bsd.own.mk>

INSTIMGBASE=	NetBSD-${DISTRIBVER}-amd64-bios-install	# gives ${IMGBASE}.img

INSTIMAGEMB?=	2250			# for all installation binaries

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
	${.CURDIR}/etc.ttys		etc/ttys			\
	${.CURDIR}/etc.rc		etc/rc				\
	${.CURDIR}/install.sh		.				\
	${.OBJDIR}/boot.cfg		.				\
	${SYSINSTDIR}/sysinstmsgs.de	.				\
	${SYSINSTDIR}/sysinstmsgs.es	.				\
	${SYSINSTDIR}/sysinstmsgs.fr	.				\
	${SYSINSTDIR}/sysinstmsgs.pl	.				\
	${SYSINSTDIR}/sysinst		.

.include "${NETBSDSRCDIR}/distrib/common/bootimage//Makefile.installimage"
