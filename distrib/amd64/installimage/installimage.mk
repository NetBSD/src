#	$NetBSD: installimage.mk,v 1.5 2025/02/02 03:19:37 tsutsui Exp $

# common code between distrib/amd64/installimage/Makefile and
# distrib/amd64/installimage-bios/Makefile.

.if ${USE_XZ_SETS:Uno} != "no"
INSTIMAGEMB?=	2500			# for all installation binaries
.else
INSTIMAGEMB?=	3800			# for all installation binaries
.endif

IMGFFSVERSION=		2
PRIMARY_BOOT=		bootxx_ffsv2
SECONDARY_BOOT=		boot
SECONDARY_BOOT_ARG=	# unnecessary

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
