#	$NetBSD: Makefile,v 1.9 2002/04/11 12:17:16 lukem Exp $

.include <bsd.own.mk>

#	create ${RAMDISK_*} variables
#
RAMDISKS=	RAMDISK_S	ramdisk-rescuesmall	\
		RAMDISK_T	ramdisk-rescuetiny

.for V F in ${RAMDISKS}
${V}DIR!=	cd ${.CURDIR}/../${F} && ${PRINTOBJDIR}
${V}=		${${V}DIR}/${F}.fs
.endfor

#	create netbsd.*.gz targets
#
#	TARGETS is a list of:
#		KERNEL_name	fs-image	"nocluster" disables clustering
#
TARGETS=	INSTALL_SMALL	${RAMDISK_S}	-	\
		INSTALL_TINY	${RAMDISK_T}	-

release:

.include "${.CURDIR}/../common/Makefile.mdset"

