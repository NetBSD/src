#	$NetBSD: Makefile,v 1.6 2002/03/19 11:50:45 reinoud Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
.include <bsd.own.mk>
.include <bsd.kernobj.mk>

KERNELS=	INSTALL 

RAMDISKDIR!=	cd $(.CURDIR)/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

CLEANFILES+=	netbsd.tmp ${KERNELS:C/^.*$/netbsd.&.gz/g}

realall:	${KERNELS:C/^.*$/netbsd.&.gz/g}

.for kern in ${KERNELS}
netbsd.${kern}.gz: ${KERNOBJDIR}/${kern}/netbsd ${RAMDISK}
	cp ${KERNOBJDIR}/${kern}/netbsd NB_INST_KNL
	${MDSETIMAGE} -v NB_INST_KNL ${RAMDISK}
	${STRIP} NB_INST_KNL
	gzip -9 NB_INST_KNL
	mv NB_INST_KNL.gz ${.TARGET}
.endfor

.include <bsd.prog.mk>
