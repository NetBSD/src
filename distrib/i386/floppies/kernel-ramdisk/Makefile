#	$NetBSD: Makefile,v 1.14 2002/04/11 12:17:16 lukem Exp $

.include <bsd.own.mk>

#	create ${RAMDISK_*} variables
#
RAMDISKS=	RAMDISK_B	ramdisk-big	\
		RAMDISK_S	ramdisk-small	\
		RAMDISK_T	ramdisk-tiny

.for V F in ${RAMDISKS}
${V}DIR!=	cd ${.CURDIR}/../${F} && ${PRINTOBJDIR}
${V}=		${${V}DIR}/${F}.fs
.endfor

#	create netbsd.*.gz targets
#
#	TARGETS is a list of:
#		KERNEL_name	fs-image	"nocluster" disables clustering
#
TARGETS=	INSTALL		${RAMDISK_B}	-		\
		INSTALL_LAPTOP	${RAMDISK_B}	-		\
		INSTALL_SMALL	${RAMDISK_S}	-		\
		INSTALL_TINY	${RAMDISK_T}	nocluster	\
		INSTALL_PS2	${RAMDISK_S}	nocluster

release: check_RELEASEDIR
	-mkdir -p ${RELEASEDIR}/binary/kernel
	cp -p ${KERNELS} ${KERNELSYMS} ${RELEASEDIR}/binary/kernel

.include "${.CURDIR}/../common/Makefile.mdset"
