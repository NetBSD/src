#	$NetBSD: Makefile,v 1.5 2002/03/12 14:28:27 reinoud Exp $

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
	cp ${KERNOBJDIR}/${kern}/netbsd netbsd.tmp
	${MDSETIMAGE} -v netbsd.tmp ${RAMDISK}
	${STRIP} netbsd.tmp
	gzip -9 netbsd.tmp
	mv netbsd.tmp.gz ${.TARGET}
.endfor

.include <bsd.prog.mk>
